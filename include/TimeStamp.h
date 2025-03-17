#pragma once

#include <iostream>
#include <string>

inline namespace muduo {

class TimeStamp {
public:
    TimeStamp();
    explicit TimeStamp(int64_t microSecondsSinceEpoch);
    static TimeStamp now();
    std::string toString() const;

private:
    int64_t microSecondsSinceEpoch_; // 自 1970-01-01 00:00:00 GMT 起的微秒数
};

}