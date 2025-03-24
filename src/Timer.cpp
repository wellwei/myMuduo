#include "Timer.h"

namespace muduo {

std::atomic<int64_t> Timer::s_numCreated_(0);

void Timer::restart(TimeStamp now) {
    if (repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = TimeStamp::invalid();
    }
}

}