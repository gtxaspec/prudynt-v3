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
std::fstream Logger::log_file;
bool Logger::have_file = false;

bool Logger::init() {
    LOG_INFO("Logger Init.");
    Logger::log_file.open("/home/wyze/logs/prudynt", std::fstream::out | std::fstream::app);
    if (!Logger::log_file.fail()) {
        have_file = true;
    }
    else {
        LOG_INFO("Failed to open log file.");
        return true;
    }
    return false;
}

void Logger::log(Level lvl, std::string module, LogMsg msg) {
    if (Logger::level < lvl) {
        return;
    }
    std::unique_lock<std::mutex> lck(log_mtx);
    std::stringstream fmt;
    fmt << "[" << text_levels[lvl] << ":" << module << "]: " << msg.log_str << std::endl;

    std::cout << fmt.str();
    if (Logger::have_file) {
        Logger::log_file << fmt.str();
        Logger::log_file.flush();
    }
}
