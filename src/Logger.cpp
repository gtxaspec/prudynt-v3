#include <iostream>
#include <unistd.h>

#define MODULE "LOGGER"

#include "Logger.hpp"

const char* text_levels[] = {
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
};

Logger::Level Logger::level = DEBUG;
std::mutex Logger::log_mtx;

Logger::Logger() {
    LOG_INFO("Logger Init.");
}

void Logger::log(Level lvl, std::string module, LogMsg msg) {
    if (Logger::level < lvl) {
        return;
    }
    std::unique_lock<std::mutex> lck(log_mtx);
    std::cout << "[" << text_levels[lvl] << ":" << module << "]: " << msg.log_str << std::endl;
}

void Logger::run() {
    while (true) {
        usleep(5000);
    }
}
