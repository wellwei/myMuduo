#include <cstdlib>

#include "Poller.h"
#include "EpollPoller.h"

namespace muduo {

Poller* Poller::newDefaultPoller(EventLoop* loop) {
    if (::getenv("MUDUO_USE_POLL")) {
        return nullptr;
    } else {
        return new EpollPoller(loop);
    }
}

}