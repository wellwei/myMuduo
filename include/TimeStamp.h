#pragma once

#include <iostream>
#include <string>

namespace muduo {

class TimeStamp {
public:
    TimeStamp() : microSecondsSinceEpoch_(0) {}

    explicit TimeStamp(int64_t microSecondsSinceEpoch) 
        : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

    static TimeStamp now();
    static TimeStamp invalid() { return TimeStamp(); }

    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    void swap(TimeStamp& that) {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    TimeStamp addTime(TimeStamp timestamp, double seconds);
    double timeDifference(TimeStamp high, TimeStamp low);

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
    time_t secondsSinceEpoch() const { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

    std::string toString() const;

    static const int kMicroSecondsPerSecond = 1000 * 1000;  // 1s = 10^6 us

private:
    int64_t microSecondsSinceEpoch_; // 自 1970-01-01 00:00:00 GMT 起的微秒数
};

inline bool operator<(TimeStamp lhs, TimeStamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(TimeStamp lhs, TimeStamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline double timeDifference(TimeStamp high, TimeStamp low) {
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / TimeStamp::kMicroSecondsPerSecond;
}

inline TimeStamp addTime(TimeStamp timestamp, double seconds) {
    int64_t delta = static_cast<int64_t>(seconds * TimeStamp::kMicroSecondsPerSecond);
    return TimeStamp(timestamp.microSecondsSinceEpoch() + delta);
}

}