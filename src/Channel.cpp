#include <sys/epoll.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

namespace muduo {

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false) {
}

Channel::~Channel() {
}

/**
 * tie机制通过 weak_ptr 跟踪 TcpConnection 的生命周期，确保在调用回调时 TcpConnection 对象是有效的，避免野指针或空指针问题。
 * 当 TcpConnection 销毁时（例如调用 connectDestroyed ），shared_ptr 引用计数减少为0，weak_ptr 无法提升，Channel 自动跳过回调执行，避免未定义行为。
 * tie 允许 Channel 和 TcpConnection 的生命周期相对独立，Channel 无需关心 TcpConnection 是否被提前销毁。
 */
void Channel::tie(const std::shared_ptr<void> &obj) {
    tie_ = obj;
    tied_ = true;
}

/**
 * 当改变 Channel 感兴趣事件时，需要调用 update() 更新 Poller 中的监听事件 (epoll_ctl)
 */
void Channel::update() {
    loop_->updateChannel(this);
}

/**
 * 从 Poller 中移除 Channel
 */
void Channel::remove() {
    loop_->removeChannel(this);
}

/**
 * 事件处理
 */
void Channel::handleEvent(TimeStamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();  // 判断 TcpConnection 是否已经销毁
        if (guard) {    // TcpConnection 存在
            handleEventWithGuard(receiveTime);
        }
        // 失败了说明 TcpConnection 已经销毁，不再处理事件
    } else {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(TimeStamp receiveTime) {
    LOG_DEBUG("Channel::handleEvent() fd = %d, revents = %d", fd_, revents_);

    // TcpConnection 通过 shutdownWrite 关闭写端后，此时 EPOLLIN 会被触发
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }

    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        if (readCallback_) readCallback_(receiveTime);
    }

    if (revents_ & EPOLLOUT) {
        if (writeCallback_) writeCallback_();
    }
}

}