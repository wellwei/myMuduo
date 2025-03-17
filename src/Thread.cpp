#include <semaphore.h>
#include <unistd.h>
#include <future>

#include "Thread.h"
#include "CurrentThread.h"

namespace muduo {

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false),
      joined_(false),
      thread_(nullptr),
      func_(std::move(func)),
      tid_(0),
      name_(name) {
    setDefaultName();
}

Thread::~Thread() {
    // 如果线程已经开始但是没有join，那么就detach
    if (started_ && !joined_) {
        thread_->detach();
    }
}

void Thread::start() {
    started_ = true;

    /**
     * 必须在 start() 函数结束前获取线程的tid，否则可能会出现线程还没有创建完成就调用了tid()函数
     * 从而导致tid_ 错误
     * 这里使用了一个信号量来保证线程的tid_在线程创建完成之后再获取
     * C++20 的 binary_semaphore 可以更好的实现这个功能
     * 
     * 为什么不用条件变量？条件变量需要额外的锁，还需要一个布尔变量来判断条件是否满足，相比之下信号量更加简单
     * 
     * 这个功能也可以通过 std::promise + std::future 来实现
     */
    sem_t sem;
    sem_init(&sem, false, 0);

    // 创建线程
    thread_ = std::make_shared<std::thread>([this, &sem] {
        tid_ = CurrentThread::tid();
        sem_post(&sem); // 通知主线程 tid_ 已经获取到了
        func_();
    });

    // 等待 tid_ 获取到
    sem_wait(&sem);
    sem_destroy(&sem);

    // std::promise<pid_t> tid_promise;
    // std::future<pid_t> tid_future = tid_promise.get_future();

    // thread_ = std::make_shared<std::thread>([&] {
    //     tid_promise.set_value(CurrentThread::tid());    // 通知主线程 tid_ 已经获取到了
    //     func_();
    // });

    // // 阻塞等待 tid_ 获取到
    // tid_ = tid_future.get();
}

void Thread::join() {
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName() {
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Thread %d", numCreated_++);
        name_ = buf;
    }
}

} // namespace muduo