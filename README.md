## muduo库 模块关系说明

关系图

![](https://gitee.com/wellme/photo/raw/master/data/202503172132559.svg)

![](https://gitee.com/wellme/photo/raw/master/data/202503172136306.svg)

模块关系说明：

1. **TcpServer**:
   - 包含一个Acceptor用于接受新连接
   - 管理多个TcpConnection对象（1:n关系）
   - 持有一个EventLoop（baseLoop）
   - 拥有一个EventLoopThreadPool线程池

2. **Acceptor**:
   - 包含一个Channel，监听连接事件
   - 接受新连接并回调TcpServer::newConnection

3. **TcpConnection**:
   - 包含一个Socket和一个Channel
   - 使用两个Buffer（inputBuffer和outputBuffer）处理数据
   - 保存本地地址和对端地址（InetAddress）
   - 属于一个EventLoop（通常是subLoop）

4. **EventLoop**:
   - 管理一个Poller
   - 处理Channel上的事件
   - 在自己的线程中运行事件循环

5. **EventLoopThreadPool**:
   - 创建和管理多个EventLoopThread
   - 提供getNextLoop方法实现负载均衡

6. **Channel**:
   - 封装文件描述符fd及其事件
   - 注册到Poller中
   - 绑定事件回调函数

7. **Poller**:
   - 底层使用epoll/poll机制
   - 管理多个Channel
   - 监听事件并通知相应Channel

8. **Buffer**:
   - 实现数据的高效读写和缓存
   - 与TcpConnection配合完成非阻塞IO





## Channel 模块

* Channel 是对 socket fd 的封装
* 事件注册：Channel 将其（socket fd）感兴趣的事件（需要监听的事件）告诉 Poller，Poller 通过 底层的多路复用机制（epoll_ctl）来监听这些事件
* 回调机制：当 Poller 监听到 socket fd 触发事件时，Channel 调用对应事件类型（EPOLLIN、EPOLLOUT）的处理函数进行处理这些事件

### 与 `TcpConnection` 的 `tie` 机制
#### tie的作用
* 安全性：tie机制通过`weak_ptr`跟踪`TcpConnection`的生命周期，确保在调用回调时`TcpConnection`对象是有效的，避免野指针或空指针问题。
* 优雅退出：当`TcpConnection`销毁时（例如调用`connectDestroyed`），`shared_ptr`引用计数减少为0，`weak_ptr`无法提升，`Channel`自动跳过回调执行，避免未定义行为。
* 解耦生命周期：tie允许`Channel`和T`cpConnection`的生命周期相对独立，`Channel`无需关心`TcpConnection`是否被提前销毁。
## Poller（Epoll实现） 模块

* 事件监听：Poller 调用 epoll_wait 阻塞等待事件，返回发生事件的 fd 列表（events_）

* 事件分发：fillActiveChannels 从 events_ 中提取 Channel（通过 data.ptr），设置 revents_，并加入 activeChannels。EventLoop 遍历 activeChannels ，调用每个 Channel 的回调函数（handleEvent）处理事件

* 流程

  ```text
  EventLoop::loop()
    -> Poller::poll()
      -> epoll_wait() 返回事件 -> fillActiveChannels()
    -> EventLoop 拿到 activeChannels
    -> Channel::handleEvent()
  ```

## EventLoop 模块

### one loop per thread 模型

每个线程最多拥有一个 EventLoop

* 主线程运行一个 EventLoop（mainReactor），负责接受连接。

* 子线程运行多个 EventLoop（subReactors），处理具体连接的 IO 事件和回调。

### 事件循环

```cpp
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping\n", this);
    while (!quit_)
    {
        activeChannels_.clear();
        pollRetureTime_ = poller_->poll(kPollTimeMs, &activeChannels_); // 获取监听结果
        for (Channel *channel : activeChannels_)	// 遍历发生事件的Channel
        {
            channel->handleEvent(pollRetureTime_);	// 执行对应的回调函数处理事件
        }
        doPendingFunctors();		// 执行其他线程提交的任务
    }
    looping_ = false;
}
```



### 流程图

```
主线程（mainLoop）                        子线程（subLoop）
   |                                     |
   | 1. 接收新连接，创建Channel             |
   |                                     |
   | 2. 调用 subLoop->queueInLoop(...)    |
   |    - 将回调添加到 pendingFunctors_    |
   |    - 调用 wakeup() 写入 eventfd      |
   |                                     |
   |                                     | 3. epoll_wait 返回（因 eventfd 可读）
   |                                     | 4. 处理事件（channel->handleEvent()）
   |                                     | 5. 执行 doPendingFunctors()（mainLoop新连接）
   |                                     |    - 执行 channel->enableReading()
   |                                     |    - 将 channel 注册到 subLoop 的epoll
```

### wakeup 机制

通过 eventfd 结合 epoll 实现跨线程唤醒（通知）,避免传统管道或信号量的大量系统调用。

#### **唤醒流程**

##### （1）**唤醒触发：写入`wakeupFd_`**

当需要唤醒`EventLoop`时（例如跨线程提交任务），调用`EventLoop::wakeup()`：

cpp

```cpp
void EventLoop::wakeup() {
    uint64_t one = 1;
    write(wakeupFd_, &one, sizeof(one)); // 向wakeupFd_写入数据
}
```

- **效果**：内核会增加`eventfd`的计数器，触发`EPOLLIN`事件。

##### （2）**事件就绪：`epoll_wait`返回**

- `EventLoop`主循环调用`poller_->poll()`阻塞在`epoll_wait`。
- 当`wakeupFd_`有数据可读时，`epoll_wait`返回，将`wakeupChannel_`加入`activeChannels_`。

##### （3）**处理唤醒事件：`handleRead()`**

cpp

```cpp
void EventLoop::handleRead() {
    uint64_t one;
    read(wakeupFd_, &one, sizeof(one)); // 读取数据，计数器归零
}
```

- 读取`wakeupFd_`的数据，使计数器归零，避免重复触发事件。
- **本质**：消费事件，确保后续`epoll_wait`能再次阻塞。

##### （4）**执行延迟任务：`doPendingFunctors()`**

- 在`loop()`中处理完所有Channel事件后，调用`doPendingFunctors()`：

cpp

```cpp
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); // 快速交换，减少锁时间
    }
    for (const Functor& functor : functors) {
        functor(); // 执行所有延迟任务
    }
}
```

- 通过`swap`快速取出任务队列，减少锁竞争。

### **为什么需要判断线程？**

```cpp
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}
```

当主线程(mainLoop)向子线程(subLoop)提交任务时，主线程不是 `subLoop` 的所属线程，因此需要将任务添加到 `subLoop` 的队列，并唤醒 `subLoop` 的线程

```cpp
// 当前所在 mainLoop 线程，isInLoopThread() == false
subLoop->runInLoop([newChannel](){
  // 将新连接绑定到子线程(subLoop)
})
```

