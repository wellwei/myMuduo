#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

#include <unistd.h>
#include <netinet/tcp.h>

namespace muduo {

Socket::~Socket() {
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr) {
    if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(struct sockaddr_in))) {
        LOG_FATAL("Socket::bindAddress sockfd_=%d failed", sockfd_);
    }
}

void Socket::listen() {
    if (0 != ::listen(sockfd_, 1024)) {
        LOG_FATAL("Socket::listen sockfd_=%d failed", sockfd_);
    }
}

int Socket::accept(InetAddress* peeraddr) {
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC); // 设置非阻塞和关闭时释放资源
    if (connfd >= 0) {
        peeraddr->setSockAddr(addr);
    } else {
        LOG_ERROR("Socket::accept sockfd_=%d failed", sockfd_);
    }

    return connfd;
}

void Socket::shutdownWrite() {
    if (0 > ::shutdown(sockfd_, SHUT_WR)) {
        LOG_ERROR("Socket::shutdownWrite sockfd_=%d failed", sockfd_);
    }
}

/*
    * SO_REUSEADDR 允许一个套接字绑定到一个已经使用的端口
    * 对于需要重启服务器并且需要绑定相同地址的情况，可以设置SO_REUSEADDR
*/
void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    if (0 != ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)))) {
        LOG_ERROR("Socket::setReuseAddr sockfd_=%d failed", sockfd_);
    }
}

/*
    * SO_REUSEPORT 允许多个套接字绑定到相同的端口
    * 适用于负载均衡的服务器程序
*/
void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    if (0 != ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof(optval)))) {
        LOG_ERROR("Socket::setReusePort sockfd_=%d failed", sockfd_);
    }
}

/*
    * SO_KEEPALIVE 启用对端的存活探测，会定期发送探测报文检测对端是否存活
    * 适用于长连接的服务器程序
*/
void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    if (0 != ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)))) {
        LOG_ERROR("Socket::setKeepAlive sockfd_=%d failed", sockfd_);
    }
}

/*
    * TCP_NODELAY 禁用Nagle算法，允许小包发送
    * 适用于实时性要求高的服务器程序
*/
void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    if (0 != ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof(optval)))) {
        LOG_ERROR("Socket::setTcpNoDelay sockfd_=%d failed", sockfd_);
    }
}

}