#include "InetAddress.h"

#include <cstring>

namespace muduo {

InetAddress::InetAddress(uint16_t port, std::string ip) {
    ::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = ::htons(port);
    addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());            // 点分十进制转换为二进制
}

std::string InetAddress::toIp() const {
    char buf[64] = "";
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));    // 二进制转换为点分十进制
    return buf;
}

std::string InetAddress::toIpPort() const {
    char buf[64] = "";
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = ::strlen(buf);
    uint16_t port = ::ntohs(addr_.sin_port);
    snprintf(buf + end, sizeof(buf) - end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const {
    return ::ntohs(addr_.sin_port);
}

}