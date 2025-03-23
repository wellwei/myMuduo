#include "TcpServer.h"
#include "Logger.h"
#include "AsyncLogger.h"
#include "TimeStamp.h"

using namespace muduo;

class EchoServer {
public:
    EchoServer(muduo::EventLoop* loop, const muduo::InetAddress& listenAddr, const std::string& nameArg)
        : server_(loop, listenAddr, nameArg) {
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        server_.setThreadNum(4);
    }

    void start() {
        server_.start();
    }

private:
    void onConnection(const muduo::TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO("new connection [%s] from %s", conn->name().c_str(), conn->peerAddress().toIpPort().c_str());
        } else {
            LOG_INFO("connection [%s] is down", conn->name().c_str());
        }
    }

    void onMessage(const muduo::TcpConnectionPtr& conn, muduo::Buffer* buf, muduo::TimeStamp time) {
        std::string msg(buf->retrieveAllAsString());
        LOG_INFO("connection [%s] recv %ld bytes at %s", conn->name().c_str(), msg.size(), time.toString().c_str());
        conn->send(msg);
    }

    TcpServer server_;
    EventLoop *loop_;
};

int main() {
    AsyncLogger logger("echoserver", 1024 * 1024 * 100, 10);
    Logger::setAsyncLogger(&logger);
    logger.start();

    EventLoop loop;
    InetAddress listenAddr(8080);
    EchoServer server(&loop, listenAddr, "EchoServer");
    server.start();
    loop.loop();

    logger.stop();
    return 0;
}