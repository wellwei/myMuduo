# myMuduo - 高性能 C++11 网络库

`myMuduo` 是一个基于 C++11 对 `muduo` 重构的轻量级、高性能 TCP 网络库。它旨在提供一个简洁、高效、易于使用的非阻塞 I/O 网络编程框架，适用于 Linux 平台。

## 设计与实现特点

`myMuduo` 库吸收了 Muduo 库的核心设计理念，并结合现代 C++ 特性进行了实现，主要特点如下：

1.  **基于 Reactor 模式:**
    *   采用经典的 **Reactor** 设计模式 (`EventLoop`, `Poller`, `Channel`) 来处理 I/O 事件，实现了高效的事件分发机制。
    *   支持 **Epoll** (`EpollPoller`) 作为底层的 I/O 多路复用技术，提供高并发处理能力。`Poller` 作为基类，未来可扩展支持其他多路复用机制（如 poll）。

2.  **主从 Reactor (多线程模型):**
    *   通过 `EventLoopThreadPool` 和 `EventLoopThread` 实现了 **"one loop per thread"** 的线程模型。
    *   一个 `main Reactor` (`EventLoop`) 负责监听和接受新连接 (`Acceptor`)。
    *   多个 `sub Reactor` (`EventLoop` 运行在独立的线程中) 负责处理已连接套接字的读写事件 (`TcpConnection`)。
    *   利用 **一致性哈希** (`ConsistenHash`) 算法将新连接 (`TcpConnection`) 均匀（或根据特定 key）地分发到 `sub Reactor` 线程池中的 `EventLoop` 上，实现负载均衡。

3.  **非阻塞 I/O 与事件驱动:**
    *   所有 I/O 操作（socket 创建、accept、read、write）均采用**非阻塞**方式。
    *   通过 `Channel` 封装文件描述符及其关心的事件（读、写、错误等）和回调函数。
    *   `EventLoop` 驱动 `Poller` 检测活动事件，并调用 `Channel` 注册的回调函数处理事件，实现**事件驱动**。

4.  **简洁的 TCP 服务端封装:**
    *   `TcpServer` 类封装了服务端的启动、连接管理和线程池配置，简化了 TCP 服务器的编写。
    *   `TcpConnection` 类封装了 TCP 连接，管理其生命周期、数据收发缓冲区 (`Buffer`) 和相关回调。

5.  **高效的缓冲区设计:**
    *   `Buffer` 类提供了自动增长的缓冲区，支持 `readv` (scatter/gather I/O) 读取数据，减少系统调用次数，并优化了内存管理（预留空间 `kCheapPrepend` 避免数据频繁移动）。

6.  **异步日志系统:**
    *   提供了高性能的**异步日志** (`AsyncLogger`, `LogFile`, `LogStream`)。
    *   前端负责格式化日志消息并将其放入缓冲区。
    *   后端日志线程负责将缓冲区中的日志数据写入文件，实现了日志记录与业务逻辑的解耦，减少对主业务流程性能的影响。
    *   支持日志级别 (`DEBUG`, `INFO`, `ERROR`, `FATAL`)、按大小滚动日志文件、定时刷新。

7.  **定时器功能:**
    *   基于 `timerfd` 实现了高效的定时器队列 (`TimerQueue`, `Timer`, `TimerId`)。
    *   支持 `runAt`, `runAfter`, `runEvery` 等接口，方便实现定时任务和超时处理。

8.  **现代 C++ 特性:**
    *   广泛使用 C++11 特性，如 `std::function`, `std::bind`, `std::shared_ptr`, `std::unique_ptr`, `std::atomic`, `lambda` 表达式等，提升了代码的简洁性、安全性和可读性。
    *   使用 `nocopyable` 基类防止对象意外拷贝。
    *   利用 RAII (Resource Acquisition Is Initialization) 管理资源（如 `Socket` 类管理文件描述符）。

9.  **线程安全与同步:**
    *   `EventLoop` 的核心操作设计为线程安全的（通过 `runInLoop` 和 `queueInLoop` 保证任务在正确的线程执行）。
    *   使用 `std::mutex`, `std::condition_variable`, `std::atomic` 等工具处理多线程同步问题。
    *   `CountDownLatch` 类提供了方便的线程同步机制。

10. **零拷贝文件传输:**
    *   `TcpConnection` 提供了 `sendFile` 接口，利用 Linux 的 `sendfile` 系统调用实现高效的零拷贝文件传输。

## 目录结构 (主要)

```
myMuduo/
├── include/          # 公共头文件 (安装后位于 /usr/local/include/myMuduo/)
│   ├── Acceptor.h
│   ├── AsyncLogger.h
│   ├── Buffer.h
│   ├── Callbacks.h
│   ├── Channel.h
│   ├── ConsistenHash.h
│   ├── CountDownLatch.h
│   ├── CurrentThread.h
│   ├── EpollPoller.h
│   ├── EventLoop.h
│   ├── EventLoopThread.h
│   ├── EventLoopThreadPool.h
│   ├── InetAddress.h
│   ├── LogFile.h
│   ├── Logger.h
│   ├── LogStream.h
│   ├── Poller.h
│   ├── Socket.h
│   ├── TcpConnection.h
│   ├── TcpServer.h
│   ├── Thread.h
│   ├── Timer.h
│   ├── TimerId.h
│   ├── TimerQueue.h
│   ├── TimeStamp.h
│   ├── copyable.h
│   └── nocopyable.h
├── src/              # 源文件实现
│   ├── Acceptor.cpp
│   ├── AsyncLogger.cpp
│   ├── Buffer.cpp
│   ├── Channel.cpp
│   ├── CountDownLatch.cpp
│   ├── DefaultPoller.cpp # 用于选择默认 Poller 实现
│   ├── EpollPoller.cpp
│   ├── EventLoop.cpp
│   ├── EventLoopThread.cpp
│   ├── EventLoopThreadPool.cpp
│   ├── InetAddress.cpp
│   ├── LogFile.cpp
│   ├── Logger.cpp
│   ├── Poller.cpp
│   ├── Socket.cpp
│   ├── TcpConnection.cpp
│   ├── TcpServer.cpp
│   ├── Thread.cpp
│   ├── Timer.cpp
│   ├── TimerQueue.cpp
│   └── TimeStamp.cpp
├── CMakeLists.txt     # 主 CMake 构建脚本
└── README.md          # 本文件
```

## 构建与安装

### 前提条件

*   CMake (版本 3.0 或更高)
*   支持 C++11 的 C++ 编译器 (如 GCC 4.8+, Clang 3.3+)
*   pthread 库 (通常 Linux 系统自带)
*   Linux 操作系统 (使用了 epoll, timerfd, eventfd 等 Linux 特定 API)

### 构建步骤

1.  **创建构建目录并进入:**

    ```bash
    mkdir -p build
    cd build
    ```

2.  **配置项目 (生成 Makefile):**

    ```bash
    # 构建 Release 版本的共享库 (推荐)
    cmake .. -DCMAKE_BUILD_TYPE=Release

    # 如果需要 Debug 版本
    # cmake .. -DCMAKE_BUILD_TYPE=Debug
    ```

3.  **编译:**

    ```bash
    make -j$(nproc) # 使用多核编译
    ```

    编译完成后，库文件 (`libmyMuduo.so` 或 `libmyMuduo.a`) 将生成在 `build/lib/` 目录下。

### 安装库

在 `build` 目录中运行:

```bash
sudo make install
```

默认情况下，库将被安装到 `/usr/local/` 目录下:

*   **头文件:** `/usr/local/include/myMuduo/`
*   **库文件:** `/usr/local/lib/`
*   **CMake 配置文件:** `/usr/local/lib/cmake/myMuduo/` (方便其他 CMake 项目查找和使用)

**安装到自定义位置:**

如果希望安装到其他路径（例如 `/home/user/myMuduo_install`），在配置项目时使用 `CMAKE_INSTALL_PREFIX` 选项:

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/your/custom/path
make -j$(nproc)
sudo make install
```

## 在其他项目中使用 myMuduo

### 方式一：使用 CMake (推荐)

如果你的项目使用 CMake 构建，这是最简单的方式。

1.  确保 `myMuduo` 库已安装到系统默认路径 (`/usr/local`) 或 CMake 能找到的路径。如果安装到自定义路径，你可能需要设置 `CMAKE_PREFIX_PATH` 环境变量或在 CMake 命令行中指定 `-DmyMuduo_DIR=/your/custom/path/lib/cmake/myMuduo`。

2.  在你的项目的 `CMakeLists.txt` 中添加以下内容：

    ```cmake
    cmake_minimum_required(VERSION 3.10) # 或更高版本
    project(YourProjectName)

    # 查找 myMuduo 包
    # REQUIRED 表示如果找不到包，CMake 会报错停止
    find_package(myMuduo REQUIRED)

    # 添加你的可执行文件或库
    add_executable(your_app your_source_files.cpp)
    # 或者 add_library(your_lib your_source_files.cpp)

    # 链接 myMuduo 库
    # myMuduo::myMuduo 是导入的目标 (Imported Target)
    target_link_libraries(your_app PRIVATE myMuduo::myMuduo)
    # 如果是库，可能是 PUBLIC 或 INTERFACE，取决于你的库设计
    # target_link_libraries(your_lib PUBLIC myMuduo::myMuduo)
    ```

### 方式二：手动链接

如果你不使用 CMake，需要手动指定头文件和库文件路径。

1.  **包含头文件:**
    在你的 C++ 源代码中，根据需要包含 `myMuduo` 的头文件：

    ```c++
    #include <myMuduo/EventLoop.h>
    #include <myMuduo/TcpServer.h>
    #include <myMuduo/Logger.h>
    // ... 其他需要的头文件
    ```

2.  **编译时链接:**
    假设 `myMuduo` 安装在默认路径 `/usr/local`：

    ```bash
    g++ -std=c++11 your_app.cpp \
        -I/usr/local/include       # 指定头文件搜索路径
        -L/usr/local/lib           # 指定库文件搜索路径
        -lmyMuduo                  # 链接 myMuduo 库
        -lpthread                  # 链接 pthread 库 (myMuduo 依赖)
        -o your_app
    ```

    如果 `myMuduo` 安装在自定义路径 `/your/custom/path`，则相应修改 `-I` 和 `-L` 参数：

    ```bash
    g++ -std=c++11 your_app.cpp \
        -I/your/custom/path/include \
        -L/your/custom/path/lib \
        -Wl,-rpath,/your/custom/path/lib # 运行时库路径 (可选但推荐)
        -lmyMuduo \
        -lpthread \
        -o your_app
    ```
    使用 `-Wl,-rpath` 可以让可执行文件在运行时找到位于非标准路径下的共享库。