#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>

#include "TimerQueue.h"
#include "Logger.h"
#include <cassert>

namespace muduo {

int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        LOG_FATAL_S << "Failed in timerfd_create";
    }
    return timerfd;
}

// 计算距离when还有多长时间(单位:ns)
struct timespec howMuchTimeFromNow(TimeStamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch() - TimeStamp::now().microSecondsSinceEpoch();
    if (microseconds < 100) {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / TimeStamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % TimeStamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

// 响应定时器到期事件，重置timerfd的可读事件，以便下一次到期事件
void readTimerfd(int timerfd, TimeStamp now) {
    uint64_t howmany;   // 读取到期次数
    ssize_t n = ::read(timerfd, &howmany, sizeof(howmany));  // 读取timerfd，以清除可读事件
    LOG_DEBUG_S << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof(howmany)) {
        LOG_ERROR_S << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }    
}

// 重置timerfd的到期时间，使用timerfd_settime函数
void resetTimerfd(int timerfd, TimeStamp expiration) {
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret) {
        LOG_ERROR_S << "timerfd_settime()";
    }
}

TimerQueue::TimerQueue(EventLoop* loop) 
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(),
      callingExpiredTimers_(false) {
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for (const Entry& timer : timers_) {
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(TimerCallback cb, TimeStamp when, double interval) {
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId) {
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer) {
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if (earliestChanged) {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancelInLoop(TimerId timerId) {
    loop_->assertInLoopThread();
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    auto it = activeTimers_.find(timer);
    if (it != activeTimers_.end()) {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        delete it->first;
        activeTimers_.erase(it);
    } else if (callingExpiredTimers_) {
        cancelingTimers_.insert(timer);
    }

    assert(timers_.size() == activeTimers_.size());
}

// 定时器到期事件处理函数
void TimerQueue::handleRead() {
    loop_->assertInLoopThread();

    TimeStamp now(TimeStamp::now());
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);   // 获取到期的定时器
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for (const Entry& entry : expired) {
        entry.second->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}

// 获取到期的定时器
std::vector<TimerQueue::Entry> TimerQueue::getExpired(TimeStamp now) {
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));   // UINTPTR_MAX：uintptr_t的最大值，这里用于构造一个合法的sentry
    TimerList::iterator end = timers_.lower_bound(sentry);  // 返回第一个未到期的定时器
    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));    // 将到期的定时器加入expired
    timers_.erase(timers_.begin(), end);
    for (const Entry& entry : expired) {
        ActiveTimer timer(entry.second, entry.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
    }
    assert(timers_.size() == activeTimers_.size());
    return expired;
}

// 重置到期的定时器(重复定时器重新加入，非重复定时器删除)
void TimerQueue::reset(const std::vector<Entry>& expired, TimeStamp now) {
    TimeStamp nextExpire;
    for (const Entry& entry : expired) {
        ActiveTimer timer(entry.second, entry.second->sequence());
        if (entry.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
            entry.second->restart(now);
            insert(entry.second);
        } else {
            delete entry.second;
        }
    }

    if (!timers_.empty()) { // 有定时器，重置timerfd的到期时间
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid()) {
        resetTimerfd(timerfd_, nextExpire);
    }
}

// 插入定时器
bool TimerQueue::insert(Timer* timer) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;   // 是否最早到期时间改变
    TimeStamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first) {
        earliestChanged = true;
    }

    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
    }

    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
    }

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}

}