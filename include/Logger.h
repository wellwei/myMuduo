#pragma once 

#include <string>

#include "nocopyable.h"

inline namespace muduo {

enum LogLevel {
        INFO,
        ERROR,
        FATAL,
        DEBUG
    };

class Logger : nocopyable {
public:
    static Logger& getInstance();
    void log(std::string msg);
    void setLogLevel(int level);

private:
    int logLevel_;
};

#define LOG_INFO(msgFormat, ...)                                \
    do {                                                        \
        Logger &Logger = Logger::getInstance();                 \
        Logger.setLogLevel(INFO);                               \
        char buf[1024];                                         \
        snprintf(buf, sizeof(buf), msgFormat, ##__VA_ARGS__);    \
        Logger.log(buf);                                        \
    } while(0)

#define LOG_ERROR(msgFormat, ...)                               \
    do {                                                        \
        Logger &Logger = Logger::getInstance();                 \
        Logger.setLogLevel(ERROR);                              \
        char buf[1024];                                         \
        snprintf(buf, sizeof(buf), msgFormat, ##__VA_ARGS__);    \
        Logger.log(buf);                                        \
    } while(0)

#define LOG_FATAL(msgFormat, ...)                               \
    do {                                                        \
        Logger &Logger = Logger::getInstance();                 \
        Logger.setLogLevel(FATAL);                              \
        char buf[1024];                                         \
        snprintf(buf, sizeof(buf), msgFormat, ##__VA_ARGS__);    \
        Logger.log(buf);                                        \
    } while(0)

#ifdef MUDEBUG
#define LOG_DEBUG(msgFormat, ...)                               \
    do {                                                        \
        Logger &Logger = Logger::getInstance();                 \
        Logger.setLogLevel(DEBUG);                             \
        char buf[1024];                                         \
        snprintf(buf, sizeof(buf), msgFormat, ##__VA_ARGS__);    \
        Logger.log(buf);                                        \
    } while(0)
#else
#define LOG_DEBUG(msgFormat, ...)
#endif

} // namespace muduo