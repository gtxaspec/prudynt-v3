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

std::mutex Encoder::sinks_lock;
std::map<uint32_t,EncoderSink> Encoder::sinks;
uint32_t Encoder::sink_id = 0;

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

    IMP_ISP_Tuning_SetWDRAttr(IMPISP_TUNING_OPS_MODE_ENABLE);
    IMP_ISP_Tuning_SetAntiFogAttr(ANTIFOG_STRONG);
    IMP_ISP_Tuning_SetAntiFlickerAttr(IMPISP_ANTIFLICKER_60HZ);

    set_day_mode(DAY_MODE_DAY);

    IMPISPDrcAttr drc;
    IMP_ISP_Tuning_GetRawDRC(&drc);
    std::cout << "drc: " << drc.mode << std::endl;
    drc.mode = IMPISP_DRC_MANUAL;
    drc.dval_min = 0x40;
    drc.dval_max = 0xFF;
    drc.slop_min = 0x40;
    drc.slop_max = 0xFF;
    drc.black_level = 0x0;
    drc.white_level = 0xFFF;
    drc.drc_strength = 0x0;
    ret = IMP_ISP_Tuning_SetRawDRC(&drc);
    std::cout << "drc: " << drc.mode << std::endl;

    return ret;
}

int Encoder::encoder_init() {
    int ret = 0;

    IMPEncoderRcAttr *rc_attr;
    IMPEncoderCHNAttr channel_attr;
    memset(&channel_attr, 0, sizeof(IMPEncoderCHNAttr));
    rc_attr = &channel_attr.rcAttr;

    channel_attr.encAttr.enType = PT_H264;
    channel_attr.encAttr.bufSize = 0;
    //0 = Baseline
    //1 = Main
    //2 = High
    //Note: The encoder seems to emit frames at half the
    //requested framerate when the profile is set to Baseline.
    //For this reason, Main or High are recommended.
    channel_attr.encAttr.profile = 2;
    channel_attr.encAttr.picWidth = 1920;
    channel_attr.encAttr.picHeight = 1080;

    channel_attr.rcAttr.outFrmRate.frmRateNum = IMP::FRAME_RATE;
    channel_attr.rcAttr.outFrmRate.frmRateDen = 1;

    //Setting maxGop to a low value causes the encoder to emit frames at a much
    //slower rate. A sufficiently low value can cause the frame emission rate to
    //drop below the frame rate.
    //I find that 2x the frame rate is a good setting.
    rc_attr->maxGop = IMP::FRAME_RATE * 2;
    {
        rc_attr->attrRcMode.rcMode = ENC_RC_MODE_SMART;
        rc_attr->attrRcMode.attrH264Smart.maxQp = 40;
        rc_attr->attrRcMode.attrH264Smart.minQp = 15;
        rc_attr->attrRcMode.attrH264Smart.staticTime = 5;
        rc_attr->attrRcMode.attrH264Smart.maxBitRate = 5000;
        rc_attr->attrRcMode.attrH264Smart.iBiasLvl = 0;
        rc_attr->attrRcMode.attrH264Smart.changePos = 50;
        rc_attr->attrRcMode.attrH264Smart.qualityLvl = 0;
        rc_attr->attrRcMode.attrH264Smart.frmQPStep = 3;
        rc_attr->attrRcMode.attrH264Smart.gopQPStep = 15;
        rc_attr->attrRcMode.attrH264Smart.gopRelation = true;
    }
    {
        /*
        rc_attr->attrRcMode.rcMode = ENC_RC_MODE_CBR;
        rc_attr->attrRcMode.attrH264Cbr.maxQp = 40;
        rc_attr->attrRcMode.attrH264Cbr.minQp = 20;
        rc_attr->attrRcMode.attrH264Cbr.outBitRate = 5000;
        rc_attr->attrRcMode.attrH264Cbr.iBiasLvl = 0;
        rc_attr->attrRcMode.attrH264Cbr.frmQPStep = 3;
        rc_attr->attrRcMode.attrH264Cbr.gopQPStep = 15;
        rc_attr->attrRcMode.attrH264Cbr.adaptiveMode = true;
        rc_attr->attrRcMode.attrH264Cbr.gopRelation = true;
        */
    }
    {
        /*
        rc_attr->attrRcMode.rcMode = ENC_RC_MODE_VBR;
        rc_attr->attrRcMode.attrH264Vbr.maxQp = 40;
        rc_attr->attrRcMode.attrH264Vbr.minQp = 20;
        rc_attr->attrRcMode.attrH264Vbr.staticTime = 5;
        rc_attr->attrRcMode.attrH264Vbr.maxBitRate = 5000;
        rc_attr->attrRcMode.attrH264Vbr.iBiasLvl = 0;
        rc_attr->attrRcMode.attrH264Vbr.changePos = 50;
        rc_attr->attrRcMode.attrH264Vbr.qualityLvl = 0;
        rc_attr->attrRcMode.attrH264Vbr.frmQPStep = 3;
        rc_attr->attrRcMode.attrH264Vbr.gopQPStep = 15;
        rc_attr->attrRcMode.attrH264Vbr.gopRelation = true;
        */
    }
    rc_attr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_HN1_TRUE;
    rc_attr->attrHSkip.hSkipAttr.m = rc_attr->maxGop - 1;
    rc_attr->attrHSkip.hSkipAttr.n = 1;
    rc_attr->attrHSkip.hSkipAttr.maxSameSceneCnt = 6;
    rc_attr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
    rc_attr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
    rc_attr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N4X;

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

void Encoder::set_day_mode(DayMode mode) {
    day_mode = mode;
    if (day_mode == DAY_MODE_DAY) {
        //Day mode sensor settings
        IMP_ISP_Tuning_SetISPRunningMode(IMPISP_RUNNING_MODE_DAY);
        IMP_ISP_Tuning_SetSceneMode(IMPISP_SCENE_MODE_LANDSCAPE);
        IMP_ISP_Tuning_SetColorfxMode(IMPISP_COLORFX_MODE_VIVID);

        IMP_ISP_Tuning_SetBrightness(128);
        IMP_ISP_Tuning_SetSaturation(150);

        //WAS manual 0x50
        //IMO anything above 0x50 produces an unacceptable amount of
        //ghosting.
        IMPISPTemperDenoiseAttr temp;
        temp.type = IMPISP_TEMPER_MANUAL;
        temp.temper_strength = 0x50;
        temp.tval_max = 0xFF;
        temp.tval_min = 0x60;
        IMP_ISP_Tuning_SetTemperDnsAttr(&temp);

        //GPIO settings
        //Enable IR filter
        GPIO::write(26, 1);
        GPIO::write(25, 0);
        //Disable IR LEDs
        GPIO::write(49, 0);
    }
    else {
        //Night mode settings
        IMP_ISP_Tuning_SetISPRunningMode(IMPISP_RUNNING_MODE_NIGHT);
        IMP_ISP_Tuning_SetSceneMode(IMPISP_SCENE_MODE_LANDSCAPE);
        IMP_ISP_Tuning_SetColorfxMode(IMPISP_COLORFX_MODE_BW);
        //We want all the exposure we can get at night
        //IMP_ISP_Tuning_SetAeComp(120);

        IMPISPSinterDenoiseAttr noise;
        noise.type = IMPISP_TUNING_OPS_TYPE_AUTO;
        noise.enable = IMPISP_TUNING_OPS_MODE_ENABLE;
        IMP_ISP_Tuning_SetSinterDnsAttr(&noise);

        IMPISPTemperDenoiseAttr temp;
        temp.type = IMPISP_TEMPER_RANGE;
        temp.temper_strength = 0x80;
        temp.tval_max = 0xFF;
        temp.tval_min = 0x10;
        IMP_ISP_Tuning_SetTemperDnsAttr(&temp);
    }
}

void Encoder::run() {
    LOG_INFO("Encoder Start.");

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

        for (unsigned int i = 0; i < stream.packCount; ++i) {
            uint8_t* start = (uint8_t*)stream.pack[i].virAddr;
            uint8_t* end = (uint8_t*)stream.pack[i].virAddr + stream.pack[i].length;

            //Write NRI bits to match RFC6184
            //Not sure if this survives live555, but can't hurt
            if (stream.pack[i].dataType.h264Type == 7 ||
                stream.pack[i].dataType.h264Type == 8 ||
                stream.pack[i].dataType.h264Type == 5) {
                start[4] |= 0x60; // NRI 11
            }
            if (stream.pack[i].dataType.h264Type == 1) {
                start[4] |= 0x40; // NRI 10
            }

#if 0
            if ((H264NALType)stream.pack[i].dataType.h264Type == H264_NAL_SPS) {
                //Baseline profile fix
                //Because the encoder outputs frames at 1/2 the requested frame
                //rate in Baseline mode, we need to double the framerate we
                //request. Unfortunately this means the encoder will produce
                //SPS NALs containing the incorrect (doubled) frame rate.
                //We fix this by dividing the SPS rate by 2.

                //XXX: We should parse the SPS rather than rely on the rate
                //always being at +24.
                *((uint8_t*)stream.pack[i].virAddr + 24) /= 2;
            }
#endif

            H264NALUnit nalu;
            nalu.imp_ts = stream.pack[i].timestamp;
            timeradd(&imp_time_base, &encode_time, &nalu.time);
            nalu.duration = 0;
            if (stream.pack[i].dataType.h264Type == 5 || stream.pack[i].dataType.h264Type == 1) {
                nalu.duration = last_nal_ts - nal_ts;
            }
            //We use start+4 because the encoder inserts 4-byte MPEG
            //'startcodes' at the beginning of each NAL. Live555 complains
            //if those are present.
            nalu.data.insert(nalu.data.end(), start+4, end);

            std::unique_lock<std::mutex> lck(Encoder::sinks_lock);
            for (std::map<uint32_t,EncoderSink>::iterator it=Encoder::sinks.begin();
                 it != Encoder::sinks.end(); ++it) {
                if (stream.pack[i].dataType.h264Type == 7 ||
                    stream.pack[i].dataType.h264Type == 8 ||
                    stream.pack[i].dataType.h264Type == 5) {
                    it->second.IDR = true;
                }
                if (it->second.IDR) {
                    while (!it->second.chn->write(nalu)) {
                        //Discard old NALUs if our sinks aren't keeping up.
                        //This prevents the MsgChannels from clogging up with
                        //old data.
                        LOG_WARN("Sink " << it->second.name << " clogged! Discarding NAL.");
                        H264NALUnit old_nal;
                        it->second.chn->read(&old_nal);
                    }
                }
            }
        }
        osd.update();
        IMP_Encoder_ReleaseStream(0, &stream);
        last_nal_ts = nal_ts;
    }
    IMP_Encoder_StopRecvPic(0);
}
