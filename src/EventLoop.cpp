#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <memory>

#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "Logger.h"

namespace muduo {

// 防止一个线程创建多个EventLoop对象
thread_local EventLoop* t_loopInThisThread = nullptr;

// IO复用接口超时时间
const int kPollTimeMs = 10000;

// 创建一个eventfd用于唤醒IO线程
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("Failed in eventfd: %d", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
        quit_(false),
        threadId_(CurrentThread::tid()),
        poller_(Poller::newDefaultPoller(this)),
        wakeupFd_(createEventfd()),
        wakeupChannel_(new Channel(this, wakeupFd_)) {
    
    LOG_DEBUG("EventLoop created %p in thread %d", this, threadId_);
    if (t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d", t_loopInThisThread, threadId_);
    } else {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));   // 设置wakeupChannel_的读回调
    wakeupChannel_->enableReading();    // 监听wakeupChannel_的读事件
}

EventLoop::~EventLoop() {
    LOG_DEBUG("EventLoop %p of thread %d destructs in thread %d", this, threadId_, CurrentThread::tid());
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping", this);

    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel* channel : activeChannels_) {
            // Poller 监听到的事件发生，调用Channel的handleEvent函数处理相应事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop中的等待执行的回调任务
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping", this);
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup();   // 唤醒 IO 线程将剩余任务执行完
    }
}

/**
 * 通过 eventfd 结合 epoll 实现跨线程唤醒（wakeup）
 * eventfd 本质上是一个计数器，可以通过 write 向其写入一个 8 字节的数据，这样 epoll 就会监听到 wakeupChannel_ 的读事件
 * 从而唤醒 IO 线程来执行相应的任务
 */
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
    }
}

/**
 * wakeupChannel_ 的读回调
 * 响应 wakeup() 函数的调用，从而唤醒 IO 线程来执行相应的任务
 */
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));   // 读取 8 字节的数据，读取成功代表成功唤醒了该EventLoop，不在乎读取的数据是什么
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {     // 如果是当前IO线程调用runInLoop，则直接执行cb
        cb();
    } else {                    // 如果是其他线程调用runInLoop，则将cb放入pendingFunctors_中，唤醒对应IO线程执行cb
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    /**
     * 如果不是当前IO线程调用queueInLoop，或者当前IO线程正在执行pendingFunctors_中的任务，则唤醒IO线程执行cb
     * 如果不判断 callingPendingFunctors_，当前IO线程正在执行pendingFunctors_中的任务时，如果不唤醒IO线程，新加入的cb将无法及时执行
     * 它会在下一次被唤醒时才会执行该 cb 任务，会造成延迟
     * */ 
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();   // 唤醒 IO 线程执行回调
    }
}

void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);    // 交换pendingFunctors_和functors，减小临界区长度
        // 交换后pendingFunctors_为空，不会阻塞其他线程往pendingFunctors_中添加任务
    }

    for (const Functor& functor : functors) {
        functor();  // 执行回调
    }

    callingPendingFunctors_ = false;
}

} // namespace muduo