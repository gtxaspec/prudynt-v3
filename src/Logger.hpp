#ifndef Logger_hpp
#define Logger_hpp

#define LOG_ERROR(str) Logger::log(Logger::ERROR, __FILE__, LogMsg() << str)
#define LOG_WARN(str) Logger::log(Logger::WARN, __FILE__, LogMsg() << str)
#define LOG_INFO(str) Logger::log(Logger::INFO, __FILE__, LogMsg() << str)
#define LOG_DEBUG(str) Logger::log(Logger::DEBUG, __FILE__, LogMsg() << str)

#include <memory>
#include <sstream>

#include "MsgChannel.hpp"

struct LogMsg {
    LogMsg() {};
    std::string log_str;
    LogMsg& operator<<(std::string a) {
        log_str.append(a);
        return *this;
    }

    LogMsg& operator<<(int a) {
        std::stringstream ss;
        ss << a;
        log_str.append(ss.str());
        return *this;
    }
};

class Logger {
public:
    Logger();

    enum Level {
        ERROR,
        WARN,
        INFO,
        DEBUG
    };

    static void log(Level level, std::string module, LogMsg msg);
private:
    static std::mutex log_mtx;
    static Level level;
};

#endif
