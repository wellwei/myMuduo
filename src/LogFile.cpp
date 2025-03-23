#include "LogFile.h"
#include <cassert>

namespace muduo {

LogFile::LogFile(const std::string& basename, size_t rollSize, bool threadSafe, int flushInterval, int checkEveryN)
    : basename_(basename),
        rollSize_(rollSize),
        flushInterval_(flushInterval),
        checkEveryN_(checkEveryN),
        count_(0),
        writtenBytes_(0),
        mutex_ (threadSafe ? new std::mutex : nullptr),
        startOfPeriod_(0),
        lastRoll_(0),
        lastFlush_(0) 
{
    assert(basename.find('/') == std::string::npos);
    rollFile();
}

void LogFile::append(const char* logline, int len) {
    if (mutex_) {
        std::lock_guard<std::mutex> lock(*mutex_);
        append_unlocked(logline, len);
    } else {
        append_unlocked(logline, len);
    }
}

void LogFile::append_unlocked(const char* logline, int len) {
    ::fwrite(logline, 1, len, file_.get());
    writtenBytes_ += len;

    if (writtenBytes_ > rollSize_) {
        rollFile();
    } else {
        ++count_;
        if (count_ >= checkEveryN_) {
            count_ = 0;
            time_t now = ::time(nullptr);
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            // 每天滚动一次
            if (thisPeriod_ != startOfPeriod_) {
                rollFile();
            } else if (now - lastFlush_ > flushInterval_) { // 每隔flushInterval_秒就flush一次
                lastFlush_ = now;
                flush();
            }
        }
    }
}

void LogFile::flush() {
    if (mutex_) {
        std::lock_guard<std::mutex> lock(*mutex_);
        ::fflush(file_.get());
    } else {
        ::fflush(file_.get());
    }
}

bool LogFile::rollFile() {
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_) {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(::fopen(filename.c_str(), "ae"));   //追加
        writtenBytes_ = 0;
        return true;
    }

    return false;
}

std::string LogFile::getLogFileName(const std::string& basename, time_t* now) {
    std::string filename;
    filename.reserve(basename.size() + 32);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(nullptr);
    gmtime_r(now, &tm);
    strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;

    filename += "log";
    return filename;
}

}