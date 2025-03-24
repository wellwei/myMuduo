#include "TimeStamp.h"

#include <sys/time.h>

namespace muduo {

TimeStamp TimeStamp::now() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);     // 获取自1970年1月1日以来的微秒数
    int64_t seconds = tv.tv_sec;
    return TimeStamp(seconds * 1000000 + tv.tv_usec);
}

std::string TimeStamp::toString() const {
    char buf[128] = {0};
    time_t second = static_cast<time_t>(microSecondsSinceEpoch_ / 1000000);
    struct tm tm_time;
    localtime_r(&second, &tm_time);     // 转换成本地时区

    snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, 
             tm_time.tm_mon + 1, 
             tm_time.tm_mday,
             tm_time.tm_hour, 
             tm_time.tm_min, 
             tm_time.tm_sec);
    return buf;
}

} // namespace muduo