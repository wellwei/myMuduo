#pragma once

#include <string>

#include "nocopyable.h"
#include <string.h>

namespace muduo {

const int kSmallBuffer = 4096;
const int kLargeBuffer = 4096 * 1024;

template<int SIZE>
class LogBuffer : nocopyable {
public:
    LogBuffer() : cur_(data_) {}

    ~LogBuffer() {}

    void append(const char* log, size_t len) {
        if (avail() > len) {
            memcpy(cur_, log, len);
            cur_ += len;
        }
    }

    const char* data() const {
        return data_;
    }

    int length() const {
        return static_cast<int>(cur_ - data_);
    }

    char* current() {
        return cur_;
    }

    size_t avail() const {
        return static_cast<size_t>(end() - cur_);
    }

    void add(size_t len) {
        cur_ += len;
    }

    void reset() {
        cur_ = data_;
    }

    void bzero() {
        memset(data_, 0, sizeof(data_));
    }

    std::string toString() const {
        return std::string(data_, length());
    }

private:
    const char* end() const {
        return data_ + sizeof(data_);
    }

    char data_[SIZE];
    char* cur_;
};

class LogStream : nocopyable {
public:
    using Buffer = LogBuffer<kLargeBuffer>;

    void append(const char* log, size_t len) {
        buffer_.append(log, len);
    }

    const Buffer& buffer() const {
        return buffer_;
    }

    void resetBuffer() {
        buffer_.reset();
    }

    LogStream& operator<<(bool v) {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    LogStream& operator<<(int v) {
        convert(v);
        return *this;
    }

    LogStream& operator<<(unsigned int v) {
        convert(v);
        return *this;
    }

    LogStream& operator<<(long v) {
        convert(v);
        return *this;
    }

    LogStream& operator<<(unsigned long v) {
        convert(v);
        return *this;
    }

    LogStream& operator<<(long long v) {
        convert(v);
        return *this;
    }

    LogStream& operator<<(unsigned long long v) {
        convert(v);
        return *this;
    }

    LogStream& operator<<(double v) {
        convert(v);
        return *this;
    }

    LogStream& operator<<(const void* p) {
        convertHex(reinterpret_cast<uintptr_t>(p));
        return *this;
    }

    LogStream& operator<<(const char* str) {
        if (str) {
            buffer_.append(str, strlen(str));
        } else {
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    LogStream& operator<<(const std::string& str) {
        buffer_.append(str.c_str(), str.size());
        return *this;
    }

    LogStream& operator<<(const Buffer& v) {
        *this << v.toString();
        return *this;
    }

    void append(const char* data, int len) {
        buffer_.append(data, len);
    }

private:
    template <typename T>
    void convert(T v) {
        std::string str = std::to_string(v);
        if (buffer_.avail() > str.size()) {
            buffer_.append(str.c_str(), str.size());
        }
    }

    void convertHex(uintptr_t v) {
        char buf[32];
        snprintf(buf, sizeof(buf), "0x%lx", v);
        if (buffer_.avail() > strlen(buf)) {
            buffer_.append(buf, strlen(buf));
        }
    }

    Buffer buffer_;
};

}