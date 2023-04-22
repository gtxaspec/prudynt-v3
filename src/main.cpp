#include <iostream>
#include <thread>

#include "MsgChannel.hpp"
#include "Encoder.hpp"
#include "RTSP.hpp"
#include "Motion.hpp"
#include "CVR.hpp"
#include "Logger.hpp"
#include "IMP.hpp"
#include "Config.hpp"

template <class T> void start_component(T c) {
    c.run();
}

Encoder enc;
RTSP rtsp;
Motion motion;
CVR cvr;

bool timesync_wait() {
    // I don't really have a better way to do this than
    // a no-earlier-than time. The most common sync failure
    // is time() == 0
    int timeout = 0;
    while (time(NULL) < 1647489843) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ++timeout;
        if (timeout == 60)
            return false;
    }
    return true;
}

int main(int argc, const char *argv[]) {
    std::thread enc_thread;
    std::thread rtsp_thread;
    std::thread motion_thread;
    std::thread cvr_thread;

    if (Logger::init()) {
        std::cout << "Logger initialization failed." << std::endl;
        return 1;
    }
    LOG_INFO("Starting Prudynt Video Server.");

    if (!timesync_wait()) {
        std::cout << "Time is not synchronized." << std::endl;
        return 1;
    }
    if (IMP::init()) {
        std::cout << "IMP initialization failed." << std::endl;
        return 1;
    }
    if (Config::singleton()->motionEnabled) {
        if (motion.init()) {
            std::cout << "Motion initialization failed." << std::endl;
            return 1;
        }
    }
    if (enc.init()) {
        std::cout << "Encoder initialization failed." << std::endl;
        return 1;
    }
    if (cvr.init()) {
        std::cout << "CVR initialization failed." << std::endl;
        return 1;
    }

    enc_thread = std::thread(start_component<Encoder>, enc);
    rtsp_thread = std::thread(start_component<RTSP>, rtsp);
    if (Config::singleton()->motionEnabled) {
        motion_thread = std::thread(start_component<Motion>, motion);
    }
    if (Config::singleton()->cvrEnabled) {
        cvr_thread = std::thread(start_component<CVR>, cvr);
    }

    enc_thread.join();
    rtsp_thread.join();
    if (Config::singleton()->motionEnabled) {
        motion_thread.join();
    }
    if (Config::singleton()->cvrEnabled) {
        cvr_thread.join();
    }
    return 0;
}
