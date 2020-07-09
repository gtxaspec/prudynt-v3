#ifndef Encoder_hpp
#define Encoder_hpp

#include <memory>
#include <imp/imp_framesource.h>
#include <imp/imp_system.h>
#include <imp/imp_isp.h>

#include "MsgChannel.hpp"
#include "Logger.hpp"

struct H264Frame {
    std::vector<uint8_t> data;
};

class Encoder {
public:
    Encoder();
    void run();

    void set_logger(std::shared_ptr<MsgChannel<LogMsg>> chn) {
        logger = chn;
    }

    void set_waterfall(std::shared_ptr<MsgChannel<H264Frame>> chn) {
        waterfall = chn;
    }
private:
    IMPSensorInfo create_sensor_info(std::string sensor);
    IMPFSChnAttr create_fs_attr();
    int system_init();
    int framesource_init();
    int encoder_init();
    std::shared_ptr<MsgChannel<LogMsg>> logger;
    std::shared_ptr<MsgChannel<H264Frame>> waterfall;
};

#endif
