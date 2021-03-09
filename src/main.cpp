#include <iostream>
#include <thread>

#include "MsgChannel.hpp"
#include "Encoder.hpp"
#include "RTSP.hpp"
#include "Logger.hpp"

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

    //Encoder -> RTSP for broadcast
    enc.connect_sink(&rtsp);

    std::thread log_thread(start_component<Logger>, logger);
    std::thread enc_thread(start_component<Encoder>, enc);
    std::thread rtsp_thread(start_component<RTSP>, rtsp);

    log_thread.join();
    enc_thread.join();
    rtsp_thread.join();
    return 0;
}
