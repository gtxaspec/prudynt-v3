#ifndef Encoder_hpp
#define Encoder_hpp

#include <memory>
#include <ctime>
#include <imp/imp_framesource.h>
#include <imp/imp_system.h>
#include <imp/imp_isp.h>

#include "MsgChannel.hpp"
#include "Logger.hpp"

struct H264NALUnit {
    std::vector<uint8_t> data;
    struct timeval time;
};

class Encoder {
public:
    Encoder();
    void run();

    void set_logger(std::shared_ptr<MsgChannel<LogMsg>> chn) {
        logger = chn;
    }

    void set_waterfall(std::shared_ptr<MsgChannel<H264NALUnit>> chn) {
        waterfall = chn;
    }
private:
    IMPSensorInfo create_sensor_info(std::string sensor);
    IMPFSChnAttr create_fs_attr();
    int system_init();
    int framesource_init();
    int encoder_init();
    std::shared_ptr<MsgChannel<LogMsg>> logger;
    std::shared_ptr<MsgChannel<H264NALUnit>> waterfall;
    struct timeval imp_time_base;
};

#endif
