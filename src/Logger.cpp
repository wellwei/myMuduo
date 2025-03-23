#include <memory>

#include "Logger.h"
#include "LogStream.h"
#include "AsyncLogger.h"
#include "TimeStamp.h"
#include <stdarg.h>

namespace muduo {

#ifndef MUDEBUG
Logger::LogLevel g_logLevel = Logger::LogLevel::INFO;
#else
Logger::LogLevel g_logLevel = Logger::LogLevel::DEBUG;
#endif

AsyncLogger* Logger::asyncLogger_ = nullptr;

Logger::Logger(SourceFile file, int line, LogLevel level) 
    : stream_(new LogStream),
        file_(file),
        line_(line),
        level_(level) {
            std::string time = TimeStamp::now().toString();
            *stream_ << time << " [" << levelToString(level) << "] " << file_.data_ << ":" << line << " ";
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* fmt, ...) 
    : stream_(new LogStream),
        file_(file),
        line_(line),
        level_(level) {
            std::string time = TimeStamp::now().toString();
            *stream_ << time << " [" << levelToString(level) << "] " << file_.data_ << ":" << line << " ";
            
            va_list args;
            va_start(args, fmt);
            char buf[1024];
            vsnprintf(buf, sizeof(buf), fmt, args);
            va_end(args);
            *stream_ << buf;
}

Logger::~Logger() {
    *stream_ << "\n";
    const LogStream::Buffer& buf(stream_->buffer());
    if (asyncLogger_) {
        asyncLogger_->append(buf.data(), buf.length());
    }
}

LogStream& Logger::stream() {
    return *stream_;
}

void Logger::setLogLevel(LogLevel level) {
    g_logLevel = level;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case Logger::LogLevel::INFO:
            return "INFO";
        case Logger::LogLevel::ERROR:
            return "ERROR";
        case Logger::LogLevel::FATAL:
            return "FATAL";
        case Logger::LogLevel::DEBUG:
            return "DEBUG";
        default:
            return "UNKNOWN";
    }
}

}