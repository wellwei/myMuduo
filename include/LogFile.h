#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <ctime>

#include "nocopyable.h"

namespace muduo {

class LogFile : nocopyable {
public:
    LogFile(const std::string& basename, size_t rollSize, bool threadSafe = true, int flushInterval = 3, int checkEveryN = 1024);

    ~LogFile() = default;
    
    void append(const char* logline, int len);

    void flush();

    bool rollFile();

private:
    void append_unlocked(const char* logline, int len);

    static std::string getLogFileName(const std::string& basename, time_t* now);

    const std::string basename_;
    const size_t rollSize_;
    const int flushInterval_;
    const int checkEveryN_;

    int count_;
    int writtenBytes_;
    std::unique_ptr<std::mutex> mutex_;
    time_t startOfPeriod_;
    time_t lastRoll_;
    time_t lastFlush_;
    std::unique_ptr<FILE> file_;

    const static int kRollPerSeconds_ = 60 * 60 * 24;   // 每天滚动一次
};

}