#include <functional>
#include <memory>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>

#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Logger.h"

namespace muduo {
static EventLoop *CheckLoopNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null.", __FILE__, __func__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
{
    LOG_DEBUG("TcpConnection::create [%s] at %p fd=%d", name_.c_str(), this, sockfd);
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG("TcpConnection::destroy [%s] at %p fd=%d state=%d", name_.c_str(), this, channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string &buf) {
    if (state_ == kConnected) {
        // 如果是单Reactor，loop_即为当前线程的EventLoop，可以直接调用sendInLoop
        // 否则需要将sendInLoop任务添加到EventLoop的任务队列中
        if (loop_->isInLoopThread()) {
            sendInLoop(buf.c_str(), buf.size());
        } else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected) {
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // 如果channel_没有关注写事件(第一次发送数据)或者output buffer没有数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;

            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {     // EWOULDBLOCK表示非阻塞情况下，写缓冲区已满 == EAGAIN
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    // 如果没有写完，将剩余数据添加到output buffer中
    if (!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);

        // 如果channel_没有关注写事件，需要关注写事件，否则会漏写数据
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    if (!channel_->isWriting()) {   // 说明output buffer中的数据已经发送完毫无疑问
        socket_->shutdownWrite();
    }
}

// 连接建立
void TcpConnection::connectEstablished() {
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed() {
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

// 当对端有数据到达时，检测到EPOLLIN事件，调用handleRead 取走数据
void TcpConnection::handleRead(TimeStamp receiveTime) {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

// 当output buffer可写时，检测到EPOLLOUT事件，调用handleWrite 发送数据
void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0) {
            outputBuffer_.retrieve(n);  // 修正偏移量
            if (outputBuffer_.readableBytes() == 0) {   // 数据发送完毕
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) { // 如果是半关闭状态，关闭连接
                    shutdownInLoop();
                }
            }
        } else {
            errno = savedErrno;
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::handleWrite");
            }
        }
    } else {
        LOG_ERROR("Connection fd = %d is down, no more writing", channel_->fd());
    }
}

void TcpConnection::handleClose() {
    LOG_DEBUG("fd = %d state = %d\n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    closeCallback_(guardThis);
}

// 错误处理
void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    int err = 0;

    // 获取socket错误码
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d", name_.c_str(), err);
}

// 零拷贝发送文件
void TcpConnection::sendFile(int fd, off_t offset, size_t count) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendFileInLoop(fd, offset, count);
        } else {
            loop_->runInLoop(std::bind(&TcpConnection::sendFileInLoop, this, fd, offset, count));
        }
    } else {
        LOG_ERROR("TcpConnection::sendFile - connection is not connected");
    }
}

// 在事件循环中执行sendfile
void TcpConnection::sendFileInLoop(int fd, off_t offset, size_t count) {
    off_t bytesSent = 0;    // 发送了多少字节
    off_t remaining = count;    // 还剩多少字节要发送
    bool faultError = false;

    if (state_ == kDisconnecting) {
        // 如果是半关闭状态，不再发送数据
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // 如果channel_没有关注写事件(第一次发送数据)或者output buffer没有数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        bytesSent = ::sendfile(channel_->fd(), fd, &offset, count);
        if (bytesSent >= 0) {
            remaining -= bytesSent;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::sendFileInLoop");
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
            if (errno == EPIPE || errno == ECONNRESET) {    // EPIPE表示对端关闭写端，ECONNRESET表示对端重置连接
                faultError = true;
            }
        }
    }

    // 没写完继续发送
    if (!faultError && remaining > 0) {
        loop_->queueInLoop(std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fd, offset, remaining));
    }
}

}