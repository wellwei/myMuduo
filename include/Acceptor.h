#pragma once 

#include <string>
#include <memory>
#include <functional>

#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "InetAddress.h"
#include "nocopyable.h"

namespace muduo {

// Acceptor类，用于监听端口，接受新的连接，并将新的连接交给用户指定的回调函数处理
// Acceptor类不负责处理新的连接，只负责接受新的连接，并将新的连接交给用户指定的回调函数处理
// 该类包含在TcpServer中，运行在mainReactor中
class Acceptor : nocopyable {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport = false);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        newConnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();  // 给acceptChannel_注册的回调函数，用于接受新的连接

    EventLoop* loop_;   // Acceptor所属的EventLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;   // 新连接的回调函数, 由TcpServer指定
    bool listenning_;
};

}