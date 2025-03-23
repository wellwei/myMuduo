#pragma once

#include "nocopyable.h"
#include "InetAddress.h"

namespace muduo {

class Socket : nocopyable {
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);
    
    void shutdownWrite();

    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
    void setTcpNoDelay(bool on);
    
private:
    const int sockfd_;
};

}