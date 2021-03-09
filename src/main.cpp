#include <iostream>
#include <thread>

#include "MsgChannel.hpp"
#include "Encoder.hpp"
#include "FrameWaterfall.hpp"
#include "Logger.hpp"

template <class T> void start_component(T c) {
    c.run();
}

Logger logger;
Encoder enc;

int main(int argc, const char *argv[]) {
    //Logging channels
    logger.connect(&enc);

    std::thread log_thread(start_component<Logger>, logger);
    std::thread enc_thread(start_component<Encoder>, enc);

    log_thread.join();
    enc_thread.join();
    return 0;
}
