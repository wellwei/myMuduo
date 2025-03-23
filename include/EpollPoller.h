#pragma once

#include <sys/epoll.h>
#include <vector>

#include "TimeStamp.h"
#include "Poller.h"

namespace muduo {

class EpollPoller : public Poller {

private:
    static const int kInitEventListSize = 16;   // epoll_wait() 的初始大小

    // 将活跃事件填充到 activeChannels 中
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    // 更新 epoll 中的事件
    void update(int operation, Channel* channel);

    using EventList = std::vector<struct epoll_event>;

    int epollfd_;
    EventList events_;  // 用于保存 epoll_wait() 返回的活跃事件

public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller() override;

    TimeStamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
};

}