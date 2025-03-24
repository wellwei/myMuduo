#include <atomic>
#include <functional>

#include "TimeStamp.h"
#include "Callbacks.h"
#include "copyable.h"

namespace muduo {

class Timer : copyable {
public:
    Timer(TimerCallback cb, TimeStamp when, double interval) 
        : callback_(std::move(cb)),
            expiration_(when),
            interval_(interval),
            repeat_(interval > 0.0),
            sequence_(s_numCreated_.fetch_add(1)) 
        {}

    void run() const { callback_(); }

    TimeStamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(TimeStamp now);

    static int64_t numCreated() { return s_numCreated_.load(); }

private:
    const TimerCallback callback_;
    TimeStamp expiration_;      // 下一次到期时间
    const double interval_;     // 周期
    const bool repeat_;        // 是否重复
    const int64_t sequence_;    // 定时器序号

    static std::atomic<int64_t> s_numCreated_;
};

}