#include <iostream>
#include <thread>

#include "MsgChannel.hpp"
#include "Encoder.hpp"
#include "FrameWaterfall.hpp"
#include "Logger.hpp"

template <class T> void start_component(T c) {
    c.run();
}

int main(int argc, const char *argv[]) {
    std::shared_ptr<Logger> log = std::make_shared<Logger>();
    std::shared_ptr<Encoder> enc = std::make_shared<Encoder>();
    std::shared_ptr<FrameWaterfall> frw = std::make_shared<FrameWaterfall>();

    //Logging channels
    std::shared_ptr<MsgChannel<LogMsg>> enc_log_chn = std::make_shared<MsgChannel<LogMsg>>();
    log->add_sender(enc_log_chn);
    enc->set_logger(enc_log_chn);

    std::shared_ptr<MsgChannel<LogMsg>> frw_log_chn = std::make_shared<MsgChannel<LogMsg>>();
    log->add_sender(frw_log_chn);
    frw->set_logger(frw_log_chn);

    //Encoder emits frames and sends them to the waterfall.
    std::shared_ptr<MsgChannel<H264Frame>> enc_waterfall_chn = std::make_shared<MsgChannel<H264Frame>>();
    frw->set_encoder(enc_waterfall_chn);
    enc->set_waterfall(enc_waterfall_chn);

    std::thread log_thread(start_component<Logger>, *log);
    std::thread enc_thread(start_component<Encoder>, *enc);
    std::thread frw_thread(start_component<FrameWaterfall>, *frw);

    log_thread.join();
    enc_thread.join();
    frw_thread.join();
    return 0;
}
