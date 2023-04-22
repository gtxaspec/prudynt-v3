#ifndef CVR_hpp
#define CVR_hpp

#include "Encoder.hpp"

class CVR {
public:
    void run();
    bool init();

    void set_framesource(std::shared_ptr<MsgChannel<H264NALUnit>> chn) {
        encoder = chn;
    }
private:
    std::string get_cvr_path(time_t t, std::string ext);
private:
    std::shared_ptr<MsgChannel<H264NALUnit>> encoder;
    uint32_t sink_id;
};

#endif