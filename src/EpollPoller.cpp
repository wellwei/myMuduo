#include <unistd.h>
#include <errno.h>
#include <cstring>

#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"

namespace muduo {

// 定义 Channel 的状态
/**
 *        添加到 epoll 中
 * kNew ====================> kAdded
 *        暂时不用（还在 channels_ 中）
 * kAdded ===================> kDeleted
 *       从 epoll 中删除，不再使用了
 * kDeleted =================> kNew
 */
const int kNew = -1;        // 新 Channel，没有添加到 epoll 中
const int kAdded = 1;       // 已经添加到 epoll 中
const int kDeleted = 2;     // 暂时不用，还在 channels_ 中

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        LOG_FATAL("EpollPoller::EpollPoller - epoll_create1() error: %d", errno);
    }
}

EpollPoller::~EpollPoller() {
    ::close(epollfd_);
}

TimeStamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_DEBUG("func = %s => fd total count %lu", __FUNCTION__, channels_.size());

    // 调用 epoll_wait() 获取发生的事件
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int savedErrno = errno;
    TimeStamp now(TimeStamp::now());

    if (numEvents > 0) {
        LOG_DEBUG("%d events happened", numEvents);

        fillActiveChannels(numEvents, activeChannels);

        // 如果活跃事件数目等于 events_ 的大小，扩容
        if (static_cast<size_t>(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_DEBUG("%s => Timeout!", __FUNCTION__);
    } else {
        // 如果是被信号中断，不打印错误日志
        if (savedErrno != EINTR) {
            errno = savedErrno;
            LOG_ERROR("EpollPoller::poll() - epoll_wait() error: %d", savedErrno);
        }
    }
    
    return now;
}

// Channel update/remove => EventLoop updateChannel/removeChannel => Poller updateChannel/removeChannel => EpollPoller updateChannel/removeChannel

// 如果
void EpollPoller::updateChannel(Channel* channel) {
    const int index = channel->index(); // 获取 Channel 在 epoll 中的状态
    LOG_DEBUG("func = %s => fd = %d events = %d index = %d", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kAdded) {
        int fd = channel->fd();

        // 如果 Channel 不再关注任何事件，调用 epoll_ctl() 从 epoll 中删除（暂时不用）
        if (channel->isNoneEvent()) {   
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    } else {
        if (index == kNew) {
            int fd = channel->fd();
            channels_[fd] = channel;
        } else {
            // kDeleted 状态的 Channel 重新添加到 epoll 中
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
}

// 不用了，置为 kNew 新态，从 channels_ 中移除
void EpollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_DEBUG("func = %s => fd = %d", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 将活跃事件填充到 activeChannels 中
void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const {
    for (int i = 0; i < numEvents; ++i) {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EpollPoller::update(int operation, Channel *channel) {
    epoll_event ev;
    ::memset(&ev, 0, sizeof(ev));

    int fd = channel->fd();

    ev.events = channel->events();
    ev.data.fd = fd;
    ev.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &ev) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("EpollPoller::update() - epoll_ctl() del error: %d", errno);
        } else {
            LOG_FATAL("EpollPoller::update() - epoll_ctl() mod/add error: %d", errno);
        }
    }
}

}