#include <iostream>
#include <cstring>
#include <unistd.h>
#include <imp/imp_log.h>
#include <imp/imp_common.h>
#include <imp/imp_encoder.h>
#include <imp/imp_osd.h>

#define MODULE "ENCODER"

#include "Encoder.hpp"

extern "C" {
    extern int IMP_OSD_SetPoolSize(int newPoolSize);
    extern int IMP_Encoder_SetPoolSize(int newPoolSize0);
}

Encoder::Encoder() {}

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
    out.pixFmt = PIX_FMT_NV12;
    out.outFrmRateNum = 12;
    out.outFrmRateDen = 1;
    out.nrVBs = 2;
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

    ret = IMP_ISP_Tuning_SetSensorFPS(12, 1);

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
    channel_attr.encAttr.profile = 1;
    channel_attr.encAttr.picWidth = 1920;
    channel_attr.encAttr.picHeight = 1080;

    //For some reason setting these values to anything other
    //than their default of zero causes channel creation to fail.
    //channel_attr.rcAttr.outFrmRate.frmRateNum = 12;
    //channel_attr.rcAttr.outFrmRate.frmRateDen = 1;

    //Setting maxGop to a low value causes the encoder to emit frames at a much
    //slower rate. A sufficiently low value can cause the frame emission rate to
    //drop below the frame rate. I assume the encoder is spitting out multiple
    //frames in each stream when that happens.
    rc_attr->maxGop = 48;
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

    IMPCell fs = { DEV_ID_FS, 0, 0 };
    IMPCell enc = { DEV_ID_ENC, 0, 0 };
    ret = IMP_System_Bind(&fs, &enc);
    if (ret < 0) {
        LOG(MODULE, "IMP_System_Bind() == " << ret);
        return;
    }

    ret = IMP_FrameSource_EnableChn(0);
    if (ret < 0) {
        LOG(MODULE, "IMP_FrameSource_EnableChn() == " << ret);
        return;
    }

    LOG(MODULE, "ISP setup complete, ready to capture frames.");

    IMP_Encoder_StartRecvPic(0);
    while (true) {
        IMPEncoderStream stream;

        if (IMP_Encoder_GetStream(0, &stream, true) != 0) {
            LOG(MODULE, "IMP_Encoder_GetStream() == " << ret);
            break;
        }

        H264Frame frame;
        for (unsigned int i = 0; i < stream.packCount; ++i) {
            frame.data.insert(
                frame.data.end(),
                (uint8_t*)stream.pack[i].virAddr,
                (uint8_t*)stream.pack[i].virAddr + stream.pack[i].length
            );
        }
        waterfall->write(frame);

        IMP_Encoder_ReleaseStream(0, &stream);
    }
    IMP_Encoder_StopRecvPic(0);
}
