#include <iostream>
#include <cstring>
#include <unistd.h>
#include <imp/imp_log.h>
#include <imp/imp_common.h>
#include <imp/imp_encoder.h>
#include <imp/imp_osd.h>
#include <ctime>

#define MODULE "ENCODER"
#define FRAME_RATE 12

#include "Encoder.hpp"

extern "C" {
    extern int IMP_OSD_SetPoolSize(int newPoolSize);
    extern int IMP_Encoder_SetPoolSize(int newPoolSize0);
}

std::mutex Encoder::sinks_lock;
std::map<uint32_t,EncoderSink> Encoder::sinks;
uint32_t Encoder::sink_id = 0;

Encoder::Encoder() : osd() {}

IMPSensorInfo Encoder::create_sensor_info(std::string sensor) {
    IMPSensorInfo out;
    memset(&out, 0, sizeof(IMPSensorInfo));
    //if (sensor.compare("jxf23") == 0) {
        LOG(MODULE, "Choosing JXF23");
        std::strcpy(out.name, "jxf23");
        out.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
        std::strcpy(out.i2c.type, "jxf23");
        out.i2c.addr = 0x40;
        return out;
    //}
}

IMPFSChnAttr Encoder::create_fs_attr() {
    IMPFSChnAttr out;
    memset(&out, 0, sizeof(IMPFSChnAttr));

    //Seems to only support the following (channel enable fails otherwise)
    //PIX_FMT_YUYV422
    //PIX_FMT_UYVY422
    //PIX_FMT_NV12
    //Of those, I have only gotten PIX_FMT_NV12 to produce frames.
    out.pixFmt = PIX_FMT_NV12;
    out.outFrmRateNum = FRAME_RATE;
    out.outFrmRateDen = 1;
    out.nrVBs = 3;
    out.type = FS_PHY_CHANNEL;
    out.crop.enable = 0;
    out.crop.top = 0;
    out.crop.left = 0;
    out.crop.width = 1920;
    out.crop.height = 1080;
    out.scaler.enable = 0;
    out.scaler.outwidth = 1920;
    out.scaler.outheight = 1080;
    out.picWidth = 1920;
    out.picHeight = 1080;
    return out;
}

int Encoder::system_init() {
    int ret = 0;

    //If you are having problems with video quality, please make
    //sure you are linking against the new version of libimp.
    //The version in /system/lib, IMP-3.11.0, is old.
    //If you see IMP-3.11.0 or lower, expect bad video
    //quality and other bugs.
    //Version IMP-3.12.0 works well in my experience.
    IMPVersion version;
    ret = IMP_System_GetVersion(&version);
    LOG(MODULE, "IMP Library Version " << version.aVersion);

    ret = IMP_ISP_Open();
    if (ret < 0) {
        LOG(MODULE, "Error: IMP_ISP_Open() == " << ret);
        return ret;
    }
    LOG(MODULE, "ISP Opened");

    IMPSensorInfo sinfo = create_sensor_info("jxf23");
    ret = IMP_ISP_AddSensor(&sinfo);
    if (ret < 0) {
        LOG(MODULE, "Error: IMP_ISP_AddSensor() == " << ret);
        return ret;
    }
    LOG(MODULE, "Sensor Added");

    ret = IMP_ISP_EnableSensor();
    if (ret < 0) {
        LOG(MODULE, "Error: IMP_ISP_EnableSensor() == " << ret);
        return ret;
    }
    LOG(MODULE, "Sensor Enabled");

    ret = IMP_System_Init();
    if (ret < 0) {
        LOG(MODULE, "Error: IMP_System_Init() == " << ret);
        return ret;
    }
    LOG(MODULE, "System Initialized");

    //Enable tuning.
    //This is necessary to customize the sensor's image output.
    //Denoising, WDR, Night Mode, and FPS customization require this.
    ret = IMP_ISP_EnableTuning();
    if (ret < 0) {
        LOG(MODULE, "ERROR: IMP_ISP_EnableTuning() == " << ret);
        return ret;
    }

    ret = IMP_ISP_Tuning_SetSensorFPS(FRAME_RATE, 1);

    return ret;
}

int Encoder::framesource_init() {
    int ret = 0;

    IMPFSChnAttr fs_chn_attr = create_fs_attr();
    ret = IMP_FrameSource_CreateChn(0, &fs_chn_attr);
    if (ret < 0) {
        LOG(MODULE, "IMP_FrameSource_CreateChn() == " << ret);
        return ret;
    }

    ret = IMP_FrameSource_SetChnAttr(0, &fs_chn_attr);
    if (ret < 0) {
        LOG(MODULE, "IMP_FrameSource_SetChnAttr() == " << ret);
        return ret;
    }

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

    channel_attr.rcAttr.outFrmRate.frmRateNum = FRAME_RATE;
    channel_attr.rcAttr.outFrmRate.frmRateDen = 1;

    //Setting maxGop to a low value causes the encoder to emit frames at a much
    //slower rate. A sufficiently low value can cause the frame emission rate to
    //drop below the frame rate.
    //I find that 2x the frame rate is a good setting.
    rc_attr->maxGop = FRAME_RATE * 2;
    rc_attr->attrRcMode.rcMode = ENC_RC_MODE_SMART;
    rc_attr->attrRcMode.attrH264Smart.maxQp = 45;
    rc_attr->attrRcMode.attrH264Smart.minQp = 15;
    rc_attr->attrRcMode.attrH264Smart.staticTime = 2;
    rc_attr->attrRcMode.attrH264Smart.maxBitRate = (unsigned int)5000;
    rc_attr->attrRcMode.attrH264Smart.iBiasLvl = 0;
    rc_attr->attrRcMode.attrH264Smart.changePos = 80;
    rc_attr->attrRcMode.attrH264Smart.qualityLvl = 0;
    rc_attr->attrRcMode.attrH264Smart.frmQPStep = 3;
    rc_attr->attrRcMode.attrH264Smart.gopQPStep = 15;
    rc_attr->attrRcMode.attrH264Smart.gopRelation = false;
    rc_attr->attrHSkip.hSkipAttr.skipType = IMP_Encoder_STYPE_N4X;
    rc_attr->attrHSkip.hSkipAttr.m = rc_attr->maxGop - 1;
    rc_attr->attrHSkip.hSkipAttr.n = 1;
    rc_attr->attrHSkip.hSkipAttr.maxSameSceneCnt = 6;
    rc_attr->attrHSkip.hSkipAttr.bEnableScenecut = 0;
    rc_attr->attrHSkip.hSkipAttr.bBlackEnhance = 0;
    rc_attr->attrHSkip.maxHSkipType = IMP_Encoder_STYPE_N4X;

    ret = IMP_Encoder_CreateChn(0, &channel_attr);
    if (ret < 0) {
        LOG(MODULE, "IMP_Encoder_CreateChn() == " << ret);
        return ret;
    }

    ret = IMP_Encoder_RegisterChn(0, 0);
    if (ret < 0) {
        LOG(MODULE, "IMP_Encoder_RegisterChn() == " << ret);
        return ret;
    }

    return ret;
}

void Encoder::run() {
    int ret = 0;
    LOG(MODULE, "Encoder Start.");

    IMP_Encoder_SetPoolSize(0x100000);

    ret = system_init();
    if (ret < 0) {
        LOG(MODULE, "System Init Failed");
        return;
    }

    ret = framesource_init();
    if (ret < 0) {
        LOG(MODULE, "Framesource Init Failed");
        return;
    }

    IMP_ISP_Tuning_SetISPBypass(IMPISP_TUNING_OPS_MODE_ENABLE);
    ret = IMP_Encoder_CreateGroup(0);
    if (ret < 0) {
        LOG(MODULE, "IMP_Encoder_CreateGroup() == " << ret);
        return;
    }

    ret = encoder_init();
    if (ret < 0) {
        LOG(MODULE, "Encoder Init Failed");
        return;
    }

    ret = osd.init();
    if (ret) {
        LOG(MODULE, "OSD Init Failed");
        return;
    }

    IMPCell fs = { DEV_ID_FS, 0, 0 };
    IMPCell osd_cell = { DEV_ID_OSD, 0, 0 };
    IMPCell enc = { DEV_ID_ENC, 0, 0 };
    //Framesource -> OSD
    ret = IMP_System_Bind(&fs, &osd_cell);
    if (ret < 0) {
        LOG(MODULE, "IMP_System_Bind(FS, OSD) == " << ret);
        return;
    }
    //OSD -> Encoder
    ret = IMP_System_Bind(&osd_cell, &enc);
    if (ret < 0) {
        LOG(MODULE, "IMP_System_Bind(OSD, ENC) == " << ret);
        return;
    }

    ret = IMP_FrameSource_EnableChn(0);
    if (ret < 0) {
        LOG(MODULE, "IMP_FrameSource_EnableChn() == " << ret);
        return;
    }

    LOG(MODULE, "ISP setup complete, ready to capture frames.");

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
            LOG(MODULE, "IMP_Encoder_GetStream() == " << ret);
            break;
        }

        //The I/P NAL is always last, but it doesn't
        //really matter which NAL we select here as they
        //all have identical timestamps.
        int64_t nal_ts = stream.pack[stream.packCount - 1].timestamp;
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
            timeradd(&imp_time_base, &encode_time, &nalu.time);
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
                        H264NALUnit old_nal;
                        it->second.chn->read(&old_nal);
                    }
                }
            }
        }
        osd.update();
        IMP_Encoder_ReleaseStream(0, &stream);
    }
    IMP_Encoder_StopRecvPic(0);
}
