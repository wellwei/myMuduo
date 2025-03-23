#pragma once

#include <memory>

#include "Buffer.h"
#include "TimeStamp.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "nocopyable.h"
#include "EventLoop.h"

namespace muduo {

class TcpConnection : nocopyable, public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    void send(const std::string& buf);
    void sendFile(int fd, off_t offset, size_t count);

    // 关闭半连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    void connectEstablished();
    void connectDestroyed();

private:
    enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected };

    void setState(StateE s) { state_ = s; }

    void handleRead(TimeStamp receiveTime); 
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *data, size_t len);
    void shutdownInLoop();
    void sendFileInLoop(int fd, off_t offset, size_t count);

    EventLoop* loop_;
    const std::string name_;
    std::atomic<StateE> state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;   // 连接时的回调函数，一般用于记录连接信息，由TcpServer传入
    MessageCallback messageCallback_;       // 读事件的回调函数，由TcpServer传入
    WriteCompleteCallback writeCompleteCallback_;   // 数据发送完毕的回调函数，由TcpServer传入
    CloseCallback closeCallback_;   // 连接关闭的回调函数，由TcpServer传入
    HighWaterMarkCallback highWaterMarkCallback_;   // 高水位回调函数，由TcpServer传入
    size_t highWaterMark_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

}