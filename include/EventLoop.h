#pragma once 

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>

#include "TimeStamp.h"
#include "nocopyable.h"
#include "CurrentThread.h"
#include "Callbacks.h"
#include "TimerId.h"
#include "Channel.h"
#include "Poller.h"
#include "TimerQueue.h"


namespace muduo {

class TimerQueue;

class EventLoop : nocopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    void runInLoop(Functor cb);     // 在当前IO线程中执行cb
    void queueInLoop(Functor cb);   // 将cb放入pendingFunctors_中，唤醒IO线程执行cb

    TimeStamp pollReturnTime() const { return pollReturnTime_; }

    void wakeup();  // 唤醒IO线程

    void updateChannel(Channel* channel);   // 更新channel
    void removeChannel(Channel* channel);   // 移除channel
    bool hasChannel(Channel* channel);      // 判断channel是否在EventLoop中

    TimerId runAt(TimeStamp time, TimerCallback cb);    // 在指定时间执行cb
    TimerId runAfter(double delay, TimerCallback cb);   // 在delay时间后执行cb
    TimerId runEvery(double interval, TimerCallback cb);    // 每隔interval时间执行cb

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    void assertInLoopThread() {
        if (!isInLoopThread()) {
            abortNotInLoopThread();
        }
    }

private:
    void abortNotInLoopThread();
    
    void handleRead();  // wakeupChannel_的读回调
    void doPendingFunctors();   // 执行pendingFunctors_中的任务

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t threadId_;

    TimeStamp pollReturnTime_;      // poll返回发生事件的时间点
    std::shared_ptr<Poller> poller_;    // IO复用器
    ChannelList activeChannels_;    // Poller返回的发生事件的Channel

    std::unique_ptr<TimerQueue> timerQueue_;    // 定时器队列

    int wakeupFd_;
    std::shared_ptr<Channel> wakeupChannel_;

    std::mutex mutex_;
    std::atomic_bool callingPendingFunctors_;   // 是否正在执行pendingFunctors_
    std::vector<Functor> pendingFunctors_;    // 存放需要在IO线程中执行的任务
};

}