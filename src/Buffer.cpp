#include <sys/uio.h>    
#include <errno.h>
#include <unistd.h>

#include "Buffer.h"

namespace muduo {

/*
    Poller 是 LT 模式，所以需要一次性将数据读完
    buffer 有大小，但是tcp 传输的数据大小是不确定的，所以需要用到额外的空间
    1. 如果预留空间足够大，可以直接使用
    2. 如果预留空间不够大，则使用额外的空间
*/
ssize_t Buffer::readFd(int fd, int *saveErrno) {
    char extrabuf[65536];   // 64KB 额外空间

    /*
        scatter/gather I/O
        分散读取，集中写入
        iovec + readv/writev 可以一次性从多个缓冲区读取数据，或者一次性写入多个缓冲区

        我们这里可能需要使用额外的空间，即读入数据到 2 个缓冲区，所以需要使用 iovec
    */
    struct iovec vec[2];
    const size_t writable = writableBytes();

    // 定义两个缓冲区的位置和大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 判断是否需要使用额外的空间
    size_t iovecnt = (writable < sizeof(extrabuf)) ? 2 : 1;

    // 一次性读取数据到两个缓冲区
    const ssize_t n = ::readv(fd, vec, iovecnt);

    if (n < 0) {
        *saveErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {    // 只使用到了 buffer_ 的空间
        writerIndex_ += n;
    } else {    // 使用了额外的空间
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());

    if (n < 0) {
        *saveErrno = errno;
    }

    return n;
}

}