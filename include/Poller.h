#pragma once

#include <vector>
#include <unordered_map>

#include "nocopyable.h"
#include "TimeStamp.h"

inline namespace muduo {

class Channel;
class EventLoop;

// IO 多路复用核心模块
class Poller : nocopyable {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller();

    // 给所有 IO 复用模块提供的接口
    virtual TimeStamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    // 判断是否有该 Channel
    bool hasChannel(Channel* channel) const;

    // EventLoop 用于获取 Poller 的实例
    static Poller* newDefaultPoller(EventLoop* loop);
    
protected:
    // key: fd, value: fd 对应的 Channel
    using ChannelMap = std::unordered_map<int, Channel*>;

    ChannelMap channels_;

private:
    EventLoop* ownerLoop_;
};

}