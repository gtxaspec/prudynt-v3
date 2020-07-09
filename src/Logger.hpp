#ifndef Logger_hpp
#define Logger_hpp

#define LOG(mod, str) logger->write(LogMsg(mod) << str)

#include <memory>
#include <sstream>

#include "MsgChannel.hpp"

struct LogMsg {
    LogMsg() {};
    LogMsg(std::string m) : module(m) {};
    std::string module;
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
    void run();
    void add_sender(std::shared_ptr<MsgChannel<LogMsg>> chn) {
        log_senders.push_back(chn);
    }

private:
    void log(LogMsg l);

    std::vector<std::shared_ptr<MsgChannel<LogMsg>>> log_senders;
};

#endif
