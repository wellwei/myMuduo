#pragma once 

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>

#include "TimeStamp.h"
#include "nocopyable.h"
#include "CurrentThread.h"


namespace muduo {

class Channel;
class Poller;

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

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:

    void handleRead();  // wakeupChannel_的读回调
    void doPendingFunctors();   // 执行pendingFunctors_中的任务

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t threadId_;

    TimeStamp pollReturnTime_;      // poll返回发生事件的时间点
    std::shared_ptr<Poller> poller_;    // IO复用器
    ChannelList activeChannels_;    // Poller返回的发生事件的Channel

    int wakeupFd_;
    std::shared_ptr<Channel> wakeupChannel_;

    std::mutex mutex_;
    std::atomic_bool callingPendingFunctors_;   // 是否正在执行pendingFunctors_
    std::vector<Functor> pendingFunctors_;    // 存放需要在IO线程中执行的任务
};

}