#ifndef RTSP_hpp
#define RTSP_hpp

#include <memory>

#include "MsgChannel.hpp"
#include "Encoder.hpp"
#include "Logger.hpp"

class RTSP {
public:
    RTSP() {};
    void run();

    void set_framesource(std::shared_ptr<MsgChannel<H264NALUnit>> chn) {
        encoder = chn;
    }
private:
    std::shared_ptr<MsgChannel<H264NALUnit>> encoder;
};

#endif
