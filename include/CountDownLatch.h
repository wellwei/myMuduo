#pragma once

#include <mutex>
#include <condition_variable>

#include "nocopyable.h"

namespace muduo {

/**
 * @brief 线程同步工具类，
 * 用于协调多个线程的执行顺序。
 * 它允许一个或多个线程等待，直到其他线程完成某些操作后才继续执行。
 */
class CountDownLatch : nocopyable {
public:
    explicit CountDownLatch(int count);

    void wait();
    
    void countDown();

    int getCount() const;

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    int count_;
};

}