#include <iostream>
#include <unistd.h>

#define MODULE "LOGGER"

#include "Logger.hpp"

Logger::Logger() {
    log(LogMsg(MODULE) << "Logger Init.");
}

void Logger::run() {
    while (true) {
        for (unsigned int i = 0; i < log_senders.size(); ++i) {
            LogMsg j;
            if (log_senders[i]->read(&j))
                log(j);
            usleep(5000);
        }
    }
}

void Logger::log(LogMsg l) {
    std::cout << "[" << l.module << "]: " << l.log_str << std::endl;
}
