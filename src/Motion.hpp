#ifndef Motion_hpp
#define Motion_hpp

#include <memory>
#include <thread>
#include <list>
#include <imp/imp_ivs.h>
#include <imp/imp_ivs_move.h>

#include "Encoder.hpp"
#include "MotionClip.hpp"
#include "MuxQueue.hpp"
#include "ListQueue.hpp"

class Motion {
public:
    void run();
    bool init();

    void set_framesource(std::shared_ptr<MsgChannel<H264NALUnit>> chn) {
        encoder = chn;
    }
public:
    static std::atomic<bool> moving;
    static std::atomic<bool> indicator;
private:
    static void detect_start(Motion *m);
    static void mux_queue_start(MuxQueue mq);
    void detect();
    void prebuffer(H264NALUnit &nal);
private:
    IMP_IVS_MoveParam move_param;
    IMPIVSInterface *move_intf;
    MuxQueue mux_queue;
    static std::thread detect_thread;
    static std::thread mux_queue_thread;
    struct timeval move_time;

    //Motion clip recording
    MotionClip *clip = nullptr;

    //Contains all NALs received from the encoder
    std::list<H264NALUnit> nalus;
    //Contains pointers into the nalus list, tracking all
    //SPS nals. This is used to identify IDRs.
    std::list<H264NALUnit*> vps;
    std::shared_ptr<MsgChannel<H264NALUnit>> encoder;
    uint32_t sink_id;
    time_t last_time;
};

#endif
