#pragma once 

#include <functional>
#include <memory>
#include <vector>
#include <string>

#include "EventLoop.h"
#include "EventLoopThread.h"
#include "ConsistenHash.h"
#include "nocopyable.h"

namespace muduo {

class EventLoopThreadPool : nocopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& name);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    EventLoop* getNextLoop(const std::string& key);

    std::vector<EventLoop*> getAllLoops();  // 返回线程池中所有EventLoop

    bool started() const { return started_; }

    const std::string& name() const { return name_; }

private:
    EventLoop* baseLoop_;  // mainLoop
    std::string name_;
    bool started_;
    int numThreads_;  // 线程池中线程数
    int next_;  // 下一个EventLoop的索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_;  // 线程池
    std::vector<EventLoop*> loops_;  // 线程池中所有EventLoop
    ConsistenHash consistenHash_;  // 一致性哈希算法
    std::unordered_map<std::string, EventLoop*> name2loop_;  // 通过name找到对应的EventLoop
};

}