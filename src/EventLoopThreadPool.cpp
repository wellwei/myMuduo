

#include "EventLoopThreadPool.h"

namespace muduo {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& name)
    : baseLoop_(baseLoop),
      name_(name),
      started_(false),
      numThreads_(0),
      next_(0),
      consistenHash_(5) {
}

EventLoopThreadPool::~EventLoopThreadPool() {
    // 此处不需要释放EventLoopThread对象，因为EventLoopThread对象是通过std::unique_ptr管理的
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    started_ = true;

    for (int i = 0; i < numThreads_; ++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
        name2loop_[buf] = loops_.back();    // 映射name到EventLoop
        consistenHash_.addNode(buf);    // 添加节点到一致性哈希环中
    }

    // 若线程池中线程数等于0（即采用单线程模型），直接调用cb
    if (numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }
}

    /**
     * @brief 若线程池中线程数大于0（即采用主从线程模型），通过一致性哈希算法选择一个EventLoop
     * 若线程池中线程数等于0（即采用单线程模型），返回baseLoop_
     */
EventLoop* EventLoopThreadPool::getNextLoop(const std::string& key) {
    if (numThreads_ > 0) {
        return name2loop_[consistenHash_.getNode(key)];     // 通过一致性哈希算法选择一个EventLoop
    } else {
        return baseLoop_;
    }
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    if (loops_.empty()) {
        return std::vector<EventLoop*>(1, baseLoop_);
    } else {
        return loops_;
    }
}

}