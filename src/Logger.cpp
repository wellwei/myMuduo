#include <iostream>
#include <string>

#include "Logger.h"
#include "TimeStamp.h"

namespace muduo {

Logger& Logger::getInstance() {
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int level) {
    logLevel_ = level;
}

// 写日志 [级别信息] time : msg
void Logger::log(std::string msg)
{
    std::string pre = "";
    switch (logLevel_)
    {
    case INFO:
        pre = "[INFO]";
        break;
    case ERROR:
        pre = "[ERROR]";
        break;
    case FATAL:
        pre = "[FATAL]";
        break;
    case DEBUG:
        pre = "[DEBUG]";
        break;
    default:
        break;
    }

    // 打印时间和msg
    std::cout << pre + TimeStamp::now().toString() << " : " << msg << std::endl;
}

}