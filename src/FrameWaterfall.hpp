#ifndef FrameWaterfall_hpp
#define FrameWaterfall_hpp

#include <memory>

#include "MsgChannel.hpp"
#include "Encoder.hpp"
#include "Logger.hpp"

class FrameWaterfall {
public:
    FrameWaterfall() {};
    void run();

    void set_encoder(std::shared_ptr<MsgChannel<H264Frame>> chn) {
        encoder = chn;
    }

    void set_logger(std::shared_ptr<MsgChannel<LogMsg>> chn) {
        logger = chn;
    }
private:
    std::shared_ptr<MsgChannel<LogMsg>> logger;
    std::shared_ptr<MsgChannel<H264Frame>> encoder;
};

#endif
