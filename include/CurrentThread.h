#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace muduo {
    namespace CurrentThread {

        // 首次调用时获取当前线程的tid，后续调用直接返回
        inline int tid() {
            thread_local static const int cachedTid = []() {
                return static_cast<int>(::gettid());
            }();
            return cachedTid;
        }

        /* 传统写法
        extern __thread int t_cachedTid;
        inline int tid() {
            if (__builtin_expect(t_cachedTid == 0, 0)) {
                t_cachedTid = static_cast<int>(::syscall(SYS_gettid));
            }
            return t_cachedTid;
        }
        */
    }
}