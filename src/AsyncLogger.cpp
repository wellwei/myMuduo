#include <cstdio>

#include "AsyncLogger.h"
#include "LogFile.h"
#include "TimeStamp.h"
#include <cassert>

namespace muduo {

AsyncLogger::AsyncLogger(const std::string& basename, size_t rollSize, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      thread_(std::bind(&AsyncLogger::threadFunc, this), "Logging"),
      latch_(1),
      mutex_(),
      cond_(),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_() {
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void AsyncLogger::append(const char* logline, int len) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentBuffer_->avail() > len) {
        currentBuffer_->append(logline, len);
    } else {
        buffers_.push_back(std::move(currentBuffer_));
        if (nextBuffer_) {
            currentBuffer_ = std::move(nextBuffer_);
        } else {
            currentBuffer_ = std::make_shared<Buffer>();
        }
        currentBuffer_->append(logline, len);
        cond_.notify_one();
    }
}

void AsyncLogger::threadFunc() {
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_, rollSize_);
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while (running_) {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (buffers_.empty()) {     // 如果没满，那么每隔flushInterval_秒就写一次
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
            }

            buffers_.push_back(std::move(currentBuffer_));  // 将currentBuffer_移动到buffers_中
            currentBuffer_ = std::move(newBuffer1); 

            buffersToWrite.swap(buffers_);  // 交换buffers_和buffersToWrite
            if (!nextBuffer_) {        // 如果备用的buffer为空，那么将newBuffer2移动到nextBuffer_
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());    // buffersToWrite不为空

        if (buffersToWrite.size() > 25) {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                     TimeStamp::now().toString().c_str(), buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        // 将buffersToWrite中的buffer写入文件
        for (const auto& buffer : buffersToWrite) {
            output.append(buffer->data(), buffer->length());
        }

        if (buffersToWrite.size() > 2) { 
            buffersToWrite.resize(2);
        }

        if (!newBuffer1) {
            if (!buffersToWrite.empty()) {
                newBuffer1 = std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer1->reset();
            } else {
                newBuffer1.reset(new Buffer);
            }
        }

        if (!newBuffer2) {
            if (!buffersToWrite.empty()) {
                newBuffer2 = std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer2->reset();
            } else {
                newBuffer2.reset(new Buffer);
            }
        }

        buffersToWrite.clear();
        output.flush();
    }

    output.flush();
}

}