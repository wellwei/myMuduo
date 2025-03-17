

#include "EventLoopThread.h"
#include "EventLoop.h"

namespace muduo {

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name) 
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb) {
      }

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

// 启动线程以及线程中的 EventLoop，返回 与该线程绑定的 EventLoop
EventLoop* EventLoopThread::startLoop() {
    thread_.start();

    EventLoop *loop = nullptr;
    {
        // 阻塞等待线程创建得到 EventLoop
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return loop_ != nullptr; });
        loop = loop_;
    }

    return loop;
}

/**
 * 在线程中运行的函数
 * 创建一个与线程绑定的 EventLoop，然后运行
 * 即 One loop per thread
 */
void EventLoopThread::threadFunc() {
    EventLoop loop;

    // 如果有线程初始化回调函数，就调用
    if (callback_) {
        callback_(&loop);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();    // 开启事件循环

    // 一直阻塞直到 loop 被 quit
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

}