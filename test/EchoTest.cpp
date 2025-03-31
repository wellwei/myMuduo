#include <gtest/gtest.h>
#include <TcpServer.h>
#include <EventLoop.h>
#include <InetAddress.h>
#include <TcpConnection.h>
#include <Buffer.h>
#include <Logger.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "AsyncLogger.h"

using namespace muduo;
using namespace std::chrono;

// 服务器端消息回调：回显收到的消息
void onMessage(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp receiveTime) {
    std::string msg = buf->retrieveAllAsString();
    conn->send(msg);
}

// 服务器端连接回调：记录连接状态
void onConnection(const TcpConnectionPtr& conn) {
}

// 客户端：发送请求并接收响应
void clientTask(int port, std::atomic<int64_t>& totalRequests, int requestsPerClient, bool keepAlive = false) {
    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(sockfd, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret = ::connect(sockfd, (sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Connect failed: %s", strerror(errno));
        ::close(sockfd);
        return;
    }

    std::string request = "Hello, mymuduo!";
    for (int i = 0; i < requestsPerClient; ++i) {
        // 发送请求
        ssize_t n = ::write(sockfd, request.c_str(), request.size());
        if (n <= 0) break;

        // 接收响应
        char buffer[1024];
        n = ::read(sockfd, buffer, sizeof(buffer));
        if (n <= 0) break;

        totalRequests.fetch_add(1, std::memory_order_relaxed);
        if (!keepAlive) break; // 短连接测试只发一次
    }
    ::close(sockfd);
}

// 测试QPS
TEST(TcpServerTest, QPS) {
    EventLoop loop;
    InetAddress listenAddr(8080);
    TcpServer server(&loop, listenAddr, "EchoServer");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.setThreadNum(8); // 使用4个线程
    server.start();

    std::thread serverThread([&loop]() { loop.loop(); });

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const int numClients = 3000; // 模拟2000个客户端
    const int requestsPerClient = 30; // 每个客户端发送20次请求
    std::atomic<int64_t> totalRequests(0);
    std::vector<std::thread> clients;

    auto start = high_resolution_clock::now();
    for (int i = 0; i < numClients; ++i) {
        clients.emplace_back(clientTask, 8080, std::ref(totalRequests), requestsPerClient, true);
    }
    for (auto& t : clients) {
        t.join();
    }
    auto end = high_resolution_clock::now();
    double duration = duration_cast<milliseconds>(end - start).count() / 1000.0; // 秒

    int64_t qps = static_cast<int64_t>(totalRequests / duration);
    LOG_INFO("QPS Test: %ld requests, %.2f seconds, QPS = %ld", totalRequests.load(), duration, qps);

    loop.quit();
    serverThread.join();

    EXPECT_GT(qps, 40000); // 期望QPS大于5000，具体值需根据硬件调整
}

// 测试并发量
TEST(TcpServerTest, Concurrency) {
    EventLoop loop;
    InetAddress listenAddr(8080);
    TcpServer server(&loop, listenAddr, "EchoServer");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.setThreadNum(8);
    server.start();

    std::thread serverThread([&loop]() { loop.loop(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const int maxConnections = 32000; // 测试10000个并发连接
    std::atomic<int64_t> totalRequests(0);
    std::vector<std::thread> clients;

    for (int i = 0; i < maxConnections; ++i) {
        clients.emplace_back(clientTask, 8080, std::ref(totalRequests), 1, true);
    }
    std::this_thread::sleep_for(std::chrono::seconds(5)); // 保持连接5秒
    for (auto& t : clients) {
        t.join();
    }

    LOG_INFO("Concurrency Test: %d connections established", maxConnections);

    loop.quit();
    serverThread.join();

    EXPECT_EQ(totalRequests.load(), maxConnections); // 期望所有连接成功
}

// 测试吞吐量
TEST(TcpServerTest, Throughput) {
    EventLoop loop;
    InetAddress listenAddr(8080);
    TcpServer server(&loop, listenAddr, "EchoServer");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback([](const TcpConnectionPtr& conn, Buffer* buf, TimeStamp) {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown(); // Ensure connection closes after response
    });
    server.setThreadNum(8); // Increase threads
    server.start();

    std::thread serverThread([&loop]() { loop.loop(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const int numClients = 500;
    const int dataSize = 1024 * 1024 * 10;
    std::atomic<int64_t> totalBytes(0);
    std::vector<std::thread> clients;
    std::string largeData(dataSize, 'A');

    auto start = high_resolution_clock::now();
    for (int i = 0; i < numClients; ++i) {
        clients.emplace_back([&](int port) {
            int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");

            if (::connect(sockfd, (sockaddr*)&addr, sizeof(addr)) >= 0) {
                ssize_t n = ::write(sockfd, largeData.c_str(), largeData.size());
                if (n > 0) totalBytes.fetch_add(n);

                fd_set readfds;
                struct timeval tv = {5, 0}; // 5-second timeout
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);
                char buffer[1024];
                size_t totalRead = 0;

                while (totalRead < largeData.size()) {
                    int ret = ::select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
                    if (ret <= 0) break;
                    n = ::read(sockfd, buffer, sizeof(buffer));
                    if (n <= 0) break;
                    totalRead += n;
                }
            }
            ::close(sockfd);
        }, 8080);
    }
    for (auto& t : clients) {
        t.join();
    }
    auto end = high_resolution_clock::now();
    double duration = duration_cast<milliseconds>(end - start).count() / 1000.0;

    double throughput = totalBytes / duration / (1024 * 1024);
    LOG_INFO("Throughput Test: %ld bytes, %.2f seconds, Throughput = %.2f MB/s", totalBytes.load(), duration, throughput);

    loop.quit();
    serverThread.join();

    EXPECT_GT(throughput, 10);
}


// 测试延迟
TEST(TcpServerTest, Latency) {
    EventLoop loop;
    InetAddress listenAddr(8080);
    TcpServer server(&loop, listenAddr, "EchoServer");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.setThreadNum(8);
    server.start();

    std::thread serverThread([&loop]() { loop.loop(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const int numRequests = 10000;
    std::vector<double> latencies;

    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ASSERT_EQ(::connect(sockfd, (sockaddr*)&addr, sizeof(addr)), 0);

    std::string request = "Ping";
    char buffer[1024];
    for (int i = 0; i < numRequests; ++i) {
        auto start = high_resolution_clock::now();
        if (::write(sockfd, request.c_str(), request.size()) < 0) {
            perror("write");
            break;
        }
        ssize_t n = ::read(sockfd, buffer, sizeof(buffer));
        auto end = high_resolution_clock::now();

        if (n > 0) {
            double latency = duration_cast<microseconds>(end - start).count() / 1000.0; // ms
            latencies.push_back(latency);
        }
    }
    ::close(sockfd);

    double avgLatency = 0;
    for (double lat : latencies) avgLatency += lat;
    avgLatency /= latencies.size();
    LOG_INFO("Latency Test: %d requests, Average Latency = %.2f ms", numRequests, avgLatency);

    loop.quit();
    serverThread.join();

    EXPECT_LT(avgLatency, 10); // 期望平均延迟小于10ms，具体值需根据硬件调整
}

int main(int argc, char **argv) {
    // muduo::AsyncLogger logger("echoserver", 1024 * 1024 * 128);
    // muduo::Logger::setAsyncLogger(&logger);
    // logger.start();

    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();

    // logger.stop();
    return ret;
}