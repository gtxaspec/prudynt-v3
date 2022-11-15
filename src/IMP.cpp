#include <iostream>
#include <cstring>
#include "IMP.hpp"

const int IMP::FRAME_RATE = 24;

bool IMP::init() {
    int ret;

    ret = system_init();
    if (ret < 0) {
        std::cout << "System Init Failed" << std::endl;
        return true;
    }

    ret = framesource_init();
    if (ret < 0) {
        std::cout << "Framesource Init Failed" << std::endl;
        return true;
    }

    IMP_ISP_Tuning_SetISPBypass(IMPISP_TUNING_OPS_MODE_ENABLE);
    return false;
}

IMPSensorInfo IMP::create_sensor_info(std::string sensor) {
    IMPSensorInfo out;
    memset(&out, 0, sizeof(IMPSensorInfo));
    //if (sensor.compare("gc2053") == 0) {
    std::cout << "Choosing GC2053" << std::endl;
        std::strcpy(out.name, "gc2053");
        out.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
        std::strcpy(out.i2c.type, "gc2053");
        out.i2c.addr = 0x37;
        return out;
    //}
}

IMPFSChnAttr IMP::create_fs_attr() {
    IMPFSChnAttr out;
    memset(&out, 0, sizeof(IMPFSChnAttr));

    //Seems to only support the following (channel enable fails otherwise)
    //PIX_FMT_YUYV422
    //PIX_FMT_UYVY422
    //PIX_FMT_NV12
    //Of those, I have only gotten PIX_FMT_NV12 to produce frames.
    out.pixFmt = PIX_FMT_NV12;
    out.outFrmRateNum = IMP::FRAME_RATE;
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

int IMP::framesource_init() {
    int ret = 0;

    IMPFSChnAttr fs_chn_attr = create_fs_attr();
    ret = IMP_FrameSource_CreateChn(0, &fs_chn_attr);
    if (ret < 0) {
        std::cout << "IMP_FrameSource_CreateChn() == " << ret << std::endl;
        return ret;
    }

    ret = IMP_FrameSource_SetChnAttr(0, &fs_chn_attr);
    if (ret < 0) {
        std::cout << "IMP_FrameSource_SetChnAttr() == " << ret << std::endl;
        return ret;
    }

    IMPFSChnFifoAttr fifo;
    IMP_FrameSource_GetChnFifoAttr(0, &fifo);
    fifo.maxdepth = 0;
    IMP_FrameSource_SetChnFifoAttr(0, &fifo);
    IMP_FrameSource_SetFrameDepth(0, 0);
    return ret;
}

int IMP::system_init() {
    int ret = 0;

    //If you are having problems with video quality, please make
    //sure you are linking against the new version of libimp.
    //The version in /system/lib, IMP-3.11.0, is old.
    //If you see IMP-3.11.0 or lower, expect bad video
    //quality and other bugs.
    //Version IMP-3.12.0 works well in my experience.
    IMPVersion version;
    ret = IMP_System_GetVersion(&version);
    std::cout << "IMP Library Version " << version.aVersion << std::endl;

    ret = IMP_ISP_Open();
    if (ret < 0) {
        std::cout << "Error: IMP_ISP_Open() == " << ret << std::endl;
        return ret;
    }
    std::cout << "ISP Opened" << std::endl;

    IMPSensorInfo sinfo = create_sensor_info("gc2053");
    ret = IMP_ISP_AddSensor(&sinfo);
    if (ret < 0) {
        std::cout << "Error: IMP_ISP_AddSensor() == " << ret << std::endl;
        return ret;
    }
    std::cout << "Sensor Added" << std::endl;

    ret = IMP_ISP_EnableSensor();
    if (ret < 0) {
        std::cout << "Error: IMP_ISP_EnableSensor() == " << ret << std::endl;
        return ret;
    }
    std::cout << "Sensor Enabled" << std::endl;

    ret = IMP_System_Init();
    if (ret < 0) {
        std::cout << "Error: IMP_System_Init() == " << ret << std::endl;
        return ret;
    }
    std::cout << "System Initialized" << std::endl;

    //Enable tuning.
    //This is necessary to customize the sensor's image output.
    //Denoising, WDR, Night Mode, and FPS customization require this.
    ret = IMP_ISP_EnableTuning();
    if (ret < 0) {
        std::cout << "ERROR: IMP_ISP_EnableTuning() == " << ret << std::endl;
        return ret;
    }

    ret = IMP_ISP_Tuning_SetSensorFPS(IMP::FRAME_RATE, 1);
    return ret;
}
