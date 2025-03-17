#pragma once

#include <functional>
#include <memory>

#include "nocopyable.h"
#include "TimeStamp.h"


inline namespace muduo {

class EventLoop;

class Channel : public nocopyable {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // 事件处理
    void handleEvent(TimeStamp receiveTime);

    void setReadCallback(const ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(const EventCallback cb) { writeCallback_ = std::move(cb); }
    void setErrorCallback(const EventCallback cb) { errorCallback_ = std::move(cb); }
    void setCloseCallback(const EventCallback cb) { closeCallback_ = std::move(cb); }

    // 生命期保证
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    // 设置 fd 要监听的事件(感兴趣的事件)
    void enableReading() { events_ |= kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回是否正在监听事件
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    // 设置 Channel 在 Poller 中的状态
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop* ownerLoop() { return loop_; }
    void remove();

private:

    void update();
    void handleEventWithGuard(TimeStamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;  // Poller 关心的文件描述符
    int events_;    // 关心的事件
    int revents_;   // Poller 返回的事件
    int index_;    // Channel 在 Poller 中的状态

    std::weak_ptr<void> tie_;   // 保证 Channel 的生命期一定晚于 TcpConnection
    bool tied_;

    // 事件回调
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};

}

