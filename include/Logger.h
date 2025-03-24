#pragma once 

#include <string>
#include <string.h>
#include <memory>

#include "LogStream.h"
#include "AsyncLogger.h"

namespace muduo {

class SourceFile {
public:
    template<int N>
    SourceFile(const char (&arr)[N])
        : data_(arr),
          size_(N-1) {
        const char* slash = strrchr(data_, '/');
        if (slash) {
            data_ = slash + 1;
            size_ -= static_cast<int>(data_ - arr);
        }
    }

    explicit SourceFile(const char* filename)
        : data_(filename) {
        const char* slash = strrchr(filename, '/');
        if (slash) {
            data_ = slash + 1;
        }
        size_ = static_cast<int>(strlen(data_));
    }

    const char* data_;
    int size_;
};

class Logger {
public:
    enum LogLevel {
        DEBUG,
        INFO,
        ERROR,
        FATAL
    };

    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char* fmt, ...);    // 格式化日志记录
    ~Logger();

    LogStream& stream();

    static void setLogLevel(LogLevel level);
    static LogLevel logLevel();
    static void setAsyncLogger(AsyncLogger* logger) {
        asyncLogger_ = logger;
    }

private:
    std::string levelToString(LogLevel level);

    std::unique_ptr<LogStream> stream_;
    SourceFile file_; 
    int line_;
    LogLevel level_;
    static AsyncLogger* asyncLogger_;
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel() {
    return g_logLevel;
}

// 日志级别宏定义（流式输出、格式化输出）

#define LOG_INFO_S if (muduo::Logger::logLevel() <= muduo::Logger::LogLevel::INFO) \
    muduo::Logger(__FILE__, __LINE__, muduo::Logger::LogLevel::INFO).stream()
#define LOG_DEBUG_S if (muduo::Logger::logLevel() <= muduo::Logger::LogLevel::DEBUG) \
    muduo::Logger(__FILE__, __LINE__, muduo::Logger::LogLevel::DEBUG).stream()
#define LOG_ERROR_S muduo::Logger(__FILE__, __LINE__, muduo::Logger::LogLevel::ERROR).stream()
#define LOG_FATAL_S muduo::Logger(__FILE__, __LINE__, muduo::Logger::LogLevel::FATAL).stream()

#define LOG_INFO(fmt, ...) if (muduo::Logger::logLevel() <= muduo::Logger::LogLevel::INFO) \
    muduo::Logger(__FILE__, __LINE__, muduo::Logger::LogLevel::INFO, fmt, ##__VA_ARGS__).stream()
#define LOG_DEBUG(fmt, ...) if (muduo::Logger::logLevel() <= muduo::Logger::LogLevel::DEBUG) \
    muduo::Logger(__FILE__, __LINE__, muduo::Logger::LogLevel::DEBUG, fmt, ##__VA_ARGS__).stream()
#define LOG_ERROR(fmt, ...) muduo::Logger(__FILE__, __LINE__, muduo::Logger::LogLevel::ERROR, fmt, ##__VA_ARGS__).stream()
#define LOG_FATAL(fmt, ...) muduo::Logger(__FILE__, __LINE__, muduo::Logger::LogLevel::FATAL, fmt, ##__VA_ARGS__).stream()

} // namespace muduo