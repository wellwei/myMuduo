#pragma once 

#include <string>
#include <vector>
#include <algorithm>

inline namespace muduo {

/**
 * @brief Buffer 类
 * @note readFd 从 对端读取数据到 buffer_ 中；通过 writeFd 将 buffer_ 中的数据写入到对端
 */
class Buffer {
public:
    static const size_t kCheapPrepend = 8;      // 前部预留空间
    static const size_t kInitialSize = 1024;    // 初始大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readerIndex_(kCheapPrepend),
          writerIndex_(kCheapPrepend) {}
    
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    const char* peek() const { return begin() + readerIndex_; }

    // 调整读指针
    void retrieve(size_t len) {
        if (len < readableBytes()) {
            readerIndex_ += len;
        } else {
            retrieveAll();
        }
    }

    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len) {
        std::string str(peek(), len);
        retrieve(len);
        return str;
    }

    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
    }

    char *beginWrite() { return begin() + writerIndex_; }
    const char *beginWrite() const { return begin() + writerIndex_; }

    void append(const char *data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    ssize_t readFd(int fd, int *savedErrno);
    ssize_t writeFd(int fd, int *savedErrno);
    
private:
    char* begin() { return &*buffer_.begin(); }
    const char* begin() const { return &*buffer_.begin(); }

    void makeSpace (size_t len) {
        // 如果可写空间加上预留空间小于 len，则扩容
        if (prependableBytes() + writableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + len);
        } else {    
            // 否则，将现有数据移动到前部预留空间，腾出足够的空间
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

}