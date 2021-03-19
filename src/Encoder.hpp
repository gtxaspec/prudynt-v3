#ifndef Encoder_hpp
#define Encoder_hpp

#include <memory>
#include <ctime>
#include <map>
#include <imp/imp_framesource.h>
#include <imp/imp_system.h>
#include <imp/imp_encoder.h>
#include <imp/imp_isp.h>

#include "MsgChannel.hpp"
#include "Logger.hpp"
#include "OSD.hpp"

struct H264NALUnit {
    std::vector<uint8_t> data;
    struct timeval time;
};

struct EncoderSink {
    std::shared_ptr<MsgChannel<H264NALUnit>> chn;
    bool IDR;
};

class Encoder {
public:
    Encoder();
    bool init();
    void run();

    static void flush() {
        IMP_Encoder_RequestIDR(0);
        IMP_Encoder_FlushStream(0);
    }

    void set_logger(std::shared_ptr<MsgChannel<LogMsg>> chn) {
        logger = chn;
    }

    template <class T> static uint32_t connect_sink(T *c) {
        std::shared_ptr<MsgChannel<H264NALUnit>> chn = std::make_shared<MsgChannel<H264NALUnit>>(10);
        std::unique_lock<std::mutex> lck(Encoder::sinks_lock);
        Encoder::sinks.insert(std::pair<uint32_t,EncoderSink>(Encoder::sink_id, {chn,false}));
        c->set_framesource(chn);
        Encoder::flush();
        return Encoder::sink_id++;
    }

    static void remove_sink(uint32_t sinkid) {
        std::unique_lock<std::mutex> lck(Encoder::sinks_lock);
        Encoder::sinks.erase(sinkid);
    }

private:
    enum DayMode {
        DAY_MODE_DAY,
        DAY_MODE_NIGHT
    };

    OSD osd;

    DayMode day_mode;
    int system_init();
    int framesource_init();
    int encoder_init();
    void set_day_mode(DayMode mode);
    std::shared_ptr<MsgChannel<LogMsg>> logger;
    static std::mutex sinks_lock;
    static uint32_t sink_id;
    static std::map<uint32_t, EncoderSink> sinks;
    struct timeval imp_time_base;
};

#endif
