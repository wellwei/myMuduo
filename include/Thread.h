#pragma once

#include <thread>
#include <functional>
#include <memory>
#include <atomic>
#include <unistd.h>
#include <string>

#include "nocopyable.h"

namespace muduo {

class Thread : nocopyable {

public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();
    
    bool started() const { return started_; }

    pid_t tid() const { return tid_; }

    const std::string& name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();

    std::atomic_bool started_;
    std::atomic_bool joined_;

    std::shared_ptr<std::thread> thread_;
    ThreadFunc func_;       // 线程要做的事情
    pid_t tid_;
    std::string name_;

    static std::atomic_int numCreated_; // 已经创建的线程数
};

}