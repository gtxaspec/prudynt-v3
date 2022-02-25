#ifndef Logger_hpp
#define Logger_hpp

#include <cstring>

#define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)
#define LOG_ERROR(str) Logger::log(Logger::ERROR, __FILENAME__, LogMsg() << str)
#define LOG_WARN(str) Logger::log(Logger::WARN, __FILENAME__, LogMsg() << str)
#define LOG_INFO(str) Logger::log(Logger::INFO, __FILENAME__, LogMsg() << str)
#define LOG_DEBUG(str) Logger::log(Logger::DEBUG, __FILENAME__, LogMsg() << str)

#include <memory>
#include <sstream>
#include <fstream>
#include <mutex>

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
    enum Level {
        ERROR,
        WARN,
        INFO,
        DEBUG
    };

    static bool init();
    static void log(Level level, std::string module, LogMsg msg);
private:
    static std::mutex log_mtx;
    static Level level;
    static std::fstream log_file;
    static bool have_file;
};

#endif
