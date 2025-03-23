#pragma once

#include <string>
#include <mutex>
#include <condition_variable>
#include <vector>

#include "Thread.h"
#include "CountDownLatch.h"
#include "LogStream.h"

namespace muduo {

class LogFile;

class AsyncLogger {
public:
    AsyncLogger(const std::string& basename, size_t rollSize, int flushInterval = 3);

    ~AsyncLogger() {
        if (running_) {
            stop();
        }
    }

    void append(const char* logline, int len);
    
    void start() {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop() {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    void threadFunc();

    using Buffer = LogBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::shared_ptr<Buffer>>;
    using BufferPtr = std::shared_ptr<Buffer>;

    const int flushInterval_;
    std::atomic<bool> running_;
    const std::string basename_;
    const size_t rollSize_;
    Thread thread_;
    CountDownLatch latch_;
    std::mutex mutex_;
    std::condition_variable cond_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};

}