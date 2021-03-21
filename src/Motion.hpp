#ifndef Motion_hpp
#define Motion_hpp

#include <memory>
#include <thread>
#include <list>
#include <imp/imp_ivs.h>
#include <imp/imp_ivs_move.h>

#include "Encoder.hpp"
#include "MotionClip.hpp"

//Maintain a prerecord buffer of (at least) this
//many seconds. When a motion event occurs, the
//prerecord buffer is written to the clip file.
#define MOTION_PRE_TIME 5

//When motion has ceased, wait this many seconds
//before we stop recording.
#define MOTION_POST_TIME 5

//If true, force the encoder to create IDRs every
//second, thus ensuring the prerecord buffer's
//duration is exactly MOTION_PRE_TIME seconds.
#define MOTION_STRICT_IDR 0

class Motion {
public:
    void run();
    bool init();

    void set_framesource(std::shared_ptr<MsgChannel<H264NALUnit>> chn) {
        encoder = chn;
    }
public:
    static std::atomic<bool> moving;
private:
    static void detect_start(Motion *m);
    void detect();
    void prebuffer(H264NALUnit &nal);
private:
    IMP_IVS_MoveParam move_param;
    IMPIVSInterface *move_intf;
    static std::thread detect_thread;
    time_t move_time;

    //Motion clip recording
    std::shared_ptr<MotionClip> clip = nullptr;

    //Contains all NALs received from the encoder
    std::list<H264NALUnit> nalus;
    //Contains pointers into the nalus list, tracking all
    //SPS nals. This is used to identify IDRs.
    std::list<H264NALUnit*> sps;
    std::shared_ptr<MsgChannel<H264NALUnit>> encoder;
    uint32_t sink_id;

#if MOTION_STRICT_IDR
    time_t last_time;
#endif
};

#endif
