#include <iostream>
#include <thread>

#include "MsgChannel.hpp"
#include "Encoder.hpp"
#include "RTSP.hpp"
#include "Motion.hpp"
#include "Logger.hpp"
#include "IMP.hpp"

template <class T> void start_component(T c) {
    c.run();
}

Encoder enc;
RTSP rtsp;
Motion motion;

int main(int argc, const char *argv[]) {
    if (Logger::init()) {
        std::cout << "Logger initialization failed." << std::endl;
        return 1;
    }
    LOG_INFO("Starting Prudynt Video Server.");

    if (IMP::init()) {
        std::cout << "IMP initialization failed." << std::endl;
        return 1;
    }
    if (motion.init()) {
        std::cout << "Motion initialization failed." << std::endl;
        return 1;
    }
    if (enc.init()) {
        std::cout << "Encoder initialization failed." << std::endl;
        return 1;
    }

    std::thread enc_thread(start_component<Encoder>, enc);
    std::thread rtsp_thread(start_component<RTSP>, rtsp);
    std::thread motion_thread(start_component<Motion>, motion);

    enc_thread.join();
    rtsp_thread.join();
    motion_thread.join();
    return 0;
}
