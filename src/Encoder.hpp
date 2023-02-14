#ifndef Encoder_hpp
#define Encoder_hpp

#include <memory>
#include <ctime>
#include <map>
#include <set>
#include <imp/imp_framesource.h>
#include <imp/imp_system.h>
#include <imp/imp_encoder.h>
#include <imp/imp_isp.h>

#include "MsgChannel.hpp"
#include "Logger.hpp"
#include "OSD.hpp"
#include "Night.hpp"

struct H264NALUnit {
    std::vector<uint8_t> data;
    struct timeval time;
    int64_t imp_ts;
    int64_t duration;
    int nal_type;
};

struct EncoderSink {
    std::shared_ptr<MsgChannel<H264NALUnit>> chn;
    bool IDR;
    bool tolerate_loss;
    std::string name;
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

    template <class T> static uint32_t connect_sink(T *c, std::string name = "Unnamed", bool tl = false) {
        LOG_DEBUG("Create Sink: " << Encoder::sink_id);
        std::shared_ptr<MsgChannel<H264NALUnit>> chn = std::make_shared<MsgChannel<H264NALUnit>>(20);
        std::unique_lock<std::mutex> lck(Encoder::sinks_mod_lock);
        Encoder::sinks_add_set.insert(std::pair<uint32_t,EncoderSink>(Encoder::sink_id, {chn, false, tl, name}));
        c->set_framesource(chn);
        Encoder::flush();
        return Encoder::sink_id++;
    }

    static void remove_sink(uint32_t sinkid) {
        LOG_DEBUG("Destroy Sink: " << sinkid);
        std::unique_lock<std::mutex> lck(Encoder::sinks_mod_lock);
        Encoder::sinks_reap_set.insert(sinkid);
    }

private:
    OSD osd;
    Night night;

    int system_init();
    int framesource_init();
    int encoder_init();
    std::map<uint32_t, EncoderSink> sinks;
    static std::atomic<uint32_t> sink_id;
    static std::mutex sinks_mod_lock;
    static std::set<uint32_t> sinks_reap_set;
    static std::map<uint32_t, EncoderSink> sinks_add_set;
    struct timeval imp_time_base;
};

#endif
