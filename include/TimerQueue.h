#pragma once

#include <set>
#include <utility>

#include "nocopyable.h"
#include "TimerId.h"
#include "EventLoop.h"
#include "Callbacks.h"
#include "TimeStamp.h"
#include "Channel.h"

namespace muduo {

// 定时器队列类，负责管理定时器
// 通过不断动态调整timerfd的到期时间，实现只用一个timerfd来管理所有定时器
class TimerQueue : nocopyable {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(TimerCallback cb, TimeStamp when, double interval);

    void cancel(TimerId timerId);

private:
    using Entry = std::pair<TimeStamp, Timer*>;
    using TimerList = std::set<Entry>;
    using ActiveTimer = std::pair<Timer*, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);

    void handleRead();
    std::vector<Entry> getExpired(TimeStamp now);
    void reset(const std::vector<Entry>& expired, TimeStamp now);

    bool insert(Timer* timer);

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList timers_;

    ActiveTimerSet activeTimers_;
    bool callingExpiredTimers_;
    ActiveTimerSet cancelingTimers_;
};

}