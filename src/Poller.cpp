

#include "Poller.h"
#include "Channel.h"

namespace muduo {

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop) {}

Poller::~Poller() {}

bool Poller::hasChannel(Channel* channel) const {
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

}