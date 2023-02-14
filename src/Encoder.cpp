#include <iostream>
#include <cstring>
#include <unistd.h>
#include <imp/imp_log.h>
#include <imp/imp_common.h>
#include <imp/imp_encoder.h>
#include <imp/imp_osd.h>
#include <ctime>

#define MODULE "ENCODER"

#include "IMP.hpp"
#include "GPIO.hpp"
#include "Encoder.hpp"

std::atomic<uint32_t> Encoder::sink_id(0);
std::mutex Encoder::sinks_mod_lock;
std::set<uint32_t> Encoder::sinks_reap_set;
std::map<uint32_t,EncoderSink> Encoder::sinks_add_set;

Encoder::Encoder() {}

bool Encoder::init() {
    int ret = 0;
    ret = IMP_Encoder_CreateGroup(0);
    if (ret < 0) {
        LOG_ERROR("IMP_Encoder_CreateGroup() == " << ret);
        return true;
    }

    ret = system_init();
    if (ret < 0) {
        LOG_ERROR("system_init failed ");
    }

    ret = encoder_init();
    if (ret < 0) {
        LOG_ERROR("Encoder Init Failed");
        return true;
    }

    ret = osd.init();
    if (ret) {
        LOG_ERROR("OSD Init Failed");
        return true;
    }

    IMPCell fs = { DEV_ID_FS, 0, 0 };
    IMPCell osd_cell = { DEV_ID_OSD, 0, 0 };
    IMPCell enc = { DEV_ID_ENC, 0, 0 };
    //Framesource -> OSD
    ret = IMP_System_Bind(&fs, &osd_cell);
    if (ret < 0) {
        LOG_ERROR("IMP_System_Bind(FS, OSD) == " << ret);
        return true;
    }
    //OSD -> Encoder
    ret = IMP_System_Bind(&osd_cell, &enc);
    if (ret < 0) {
        LOG_ERROR("IMP_System_Bind(OSD, ENC) == " << ret);
        return true;
    }

    ret = IMP_FrameSource_EnableChn(0);
    if (ret < 0) {
        LOG_ERROR("IMP_FrameSource_EnableChn() == " << ret);
        return true;
    }

    return false;
}

int Encoder::system_init() {
    int ret = 0;

    IMP_ISP_Tuning_SetAntiFlickerAttr(IMPISP_ANTIFLICKER_60HZ);
    night.init();

    return ret;
}

int Encoder::encoder_init() {
    int ret = 0;

    IMPEncoderRcAttr *rc_attr;
    IMPEncoderChnAttr channel_attr;
    memset(&channel_attr, 0, sizeof(IMPEncoderChnAttr));
    rc_attr = &channel_attr.rcAttr;

    IMP_Encoder_SetDefaultParam(
        &channel_attr, IMP_ENC_PROFILE_HEVC_MAIN, IMP_ENC_RC_MODE_VBR, 1920, 1080,
        IMP::FRAME_RATE, 1, IMP::FRAME_RATE * 2, 1,
        -1, 1000
    );

    ret = IMP_Encoder_CreateChn(0, &channel_attr);
    if (ret < 0) {
        LOG_ERROR("IMP_Encoder_CreateChn() == " << ret);
        return ret;
    }

    ret = IMP_Encoder_RegisterChn(0, 0);
    if (ret < 0) {
        LOG_ERROR("IMP_Encoder_RegisterChn() == " << ret);
        return ret;
    }

    return ret;
}

void Encoder::run() {
    LOG_INFO("Encoder Start.");

    //The encoder thread is very important, but we
    //want sink threads to have higher priority.
    nice(-19);

    int64_t last_nal_ts = 0;

    //The encoder tracks NAL timestamps with an int64_t.
    //INT64_MAX = 9,223,372,036,854,775,807
    //That means the encoder won't overflow its timestamp unless
    //this program is left running for more than 106,751,991 days,
    //or nearly 300,000 years. I think it's okay if we don't
    //handle timestamp overflows. :)
    IMP_System_RebaseTimeStamp(0);
    gettimeofday(&imp_time_base, NULL);
    IMP_Encoder_StartRecvPic(0);
    while (true) {
        IMPEncoderStream stream;

        if (IMP_Encoder_GetStream(0, &stream, true) != 0) {
            LOG_ERROR("IMP_Encoder_GetStream() failed");
            break;
        }

        //The I/P NAL is always last, but it doesn't
        //really matter which NAL we select here as they
        //all have identical timestamps.
        int64_t nal_ts = stream.pack[stream.packCount - 1].timestamp;
        if (nal_ts - last_nal_ts > 1.5*(1000000/IMP::FRAME_RATE)) {
            LOG_WARN("The encoder dropped a frame.");
        }
        struct timeval encode_time;
        encode_time.tv_sec  = nal_ts / 1000000;
        encode_time.tv_usec = nal_ts % 1000000;

        std::vector<H264NALUnit> packs;
        for (unsigned int i = 0; i < stream.packCount; ++i) {
            uint8_t* start = (uint8_t*)stream.virAddr + stream.pack[i].offset;
            uint8_t* end = start + stream.pack[i].length;

            H264NALUnit nalu;
            nalu.nal_type = stream.pack[i].nalType.h265NalType;
            nalu.imp_ts = stream.pack[i].timestamp;
            timeradd(&imp_time_base, &encode_time, &nalu.time);
            nalu.duration = 0;
            if (nalu.nal_type == 19 ||
                nalu.nal_type == 20 ||
                nalu.nal_type == 1) {
                nalu.duration = last_nal_ts - nal_ts;
            }
            //We use start+4 because the encoder inserts 4-byte MPEG
            //'startcodes' at the beginning of each NAL. Live555 complains
            //if those are present.
            nalu.data.insert(nalu.data.end(), start+4, end);
            packs.push_back(nalu);
        }

        osd.update();
        night.update();
        IMP_Encoder_ReleaseStream(0, &stream);

        //Send nalus to sinks
        for (auto pit = packs.begin(); pit != packs.end(); ++pit) {
            for (std::map<uint32_t,EncoderSink>::iterator it=sinks.begin();
                 it != sinks.end(); ++it) {
                if (pit->nal_type == 32) {
                    it->second.IDR = true;
                }
                if (it->second.IDR) {
                    while (!it->second.chn->write(*pit)) {
                        //Some sinks can tolerate nalu loss.
                        //Don't yield for these sinks.
                        if (it->second.tolerate_loss) {
                            break;
                        }
                        //Sink is clogged. Let it catch up by yielding.
                        std::this_thread::yield();
                        //Another thread could have deleted the sink, check.
                        std::unique_lock<std::mutex> lck(Encoder::sinks_mod_lock);
                        if (Encoder::sinks_reap_set.find(it->first) != Encoder::sinks_reap_set.end()) {
                            LOG_INFO("Sink deleted during yield loop, break.");
                            break;
                        }
                    }
                }
            }
        }

        //Add requested sinks
        Encoder::sinks_mod_lock.lock();
        for (auto it = Encoder::sinks_add_set.begin();
             it != Encoder::sinks_add_set.end(); ++it) {
            sinks.insert(*it);
        }
        Encoder::sinks_add_set.clear();
        //Delete sinks marked for removal
        for (auto it = Encoder::sinks_reap_set.begin();
             it != Encoder::sinks_reap_set.end(); ++it) {
            sinks.erase(*it);
        }
        Encoder::sinks_reap_set.clear();
        Encoder::sinks_mod_lock.unlock();

        last_nal_ts = nal_ts;
        std::this_thread::yield();
    }
    IMP_Encoder_StopRecvPic(0);
}
