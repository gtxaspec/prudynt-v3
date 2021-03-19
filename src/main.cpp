#include <iostream>
#include <thread>

#include "MsgChannel.hpp"
#include "Encoder.hpp"
#include "RTSP.hpp"
#include "Logger.hpp"
#include "IMP.hpp"

template <class T> void start_component(T c) {
    c.run();
}

Logger logger;
Encoder enc;
RTSP rtsp;

int main(int argc, const char *argv[]) {
    //Logging channels
    logger.connect(&enc);
    logger.connect(&rtsp);

    if (IMP::init()) {
        std::cout << "IMP initialization failed." << std::endl;
        return 1;
    }
    /*if (motion.init()) {
        std::cout << "Motion initialization failed." << std::endl;
        return 1;
    }*/
    if (enc.init()) {
        std::cout << "Encoder initialization failed." << std::endl;
        return 1;
    }

    std::thread log_thread(start_component<Logger>, logger);
    std::thread enc_thread(start_component<Encoder>, enc);
    std::thread rtsp_thread(start_component<RTSP>, rtsp);

    log_thread.join();
    enc_thread.join();
    rtsp_thread.join();
    return 0;
}
