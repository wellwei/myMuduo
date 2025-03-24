#pragma once

#include "copyable.h"
#include "Timer.h"

namespace muduo {

class TimerId : copyable {
public:
    TimerId() : timer_(nullptr), sequence_(0) {}

    TimerId(Timer* timer, int64_t seq) : timer_(timer), sequence_(seq) {}

    friend class TimerQueue;

private:
    Timer* timer_;
    int64_t sequence_;
};

}