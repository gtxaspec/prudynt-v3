#include <iostream>
#include "Motion.hpp"

std::atomic<bool> Motion::moving;
std::thread Motion::detect_thread;
void Motion::detect_start(Motion *m) { m->detect(); }

void Motion::detect() {
    LOG_INFO("Detection thread started");
    int ret;
    IMP_IVS_MoveOutput *result;
    while (true) {
        ret = IMP_IVS_PollingResult(0, IMP_IVS_DEFAULT_TIMEOUTMS);
        if (ret < 0) {
            LOG_WARN("IMP_IVS_PollingResult error: " << ret);
            continue;
        }

        ret = IMP_IVS_GetResult(0, (void**)&result);
        if (ret < 0) {
            LOG_WARN("IMP_IVS_GetResult error: " << ret);
            continue;
        }

        struct timeval now, diff;
        gettimeofday(&now, NULL);
        if (result->retRoi[0]) {
            if (!Motion::moving) {
                LOG_INFO("Motion Start");
                Motion::moving = true;
            }
            gettimeofday(&move_time, NULL);
        }
        else {
            timersub(&now, &move_time, &diff);
            if (Motion::moving && diff.tv_sec >= MOTION_POST_TIME) {
                LOG_INFO("End of Motion");
                Motion::moving = false;
            }
        }

        ret = IMP_IVS_ReleaseResult(0, (void**)&result);
        if (ret < 0) {
            return;
        }
    }
}

bool Motion::init() {
    int ret;
    IMP_IVS_MoveParam move_param;
    IMPIVSInterface *move_intf;
    ret = IMP_IVS_CreateGroup(0);
    if (ret < 0) {
        LOG_ERROR("IMP_IVS_CreateGroup() == " << ret);
        return true;
    }

    memset(&move_param, 0, sizeof(IMP_IVS_MoveParam));
    //OSD is affecting motion for some reason.
    //Sensitivity range is 0-4
    move_param.sense[0] = 2;
    move_param.skipFrameCnt = 0;
    move_param.frameInfo.width = 1920;
    move_param.frameInfo.height = 1080;
    move_param.roiRect[0].p0.x = 0;
    move_param.roiRect[0].p0.y = 0;
    move_param.roiRect[0].p1.x = 1920 - 1;
    move_param.roiRect[0].p1.y = 1080 - 1 - 0;
    move_param.roiRectCnt = 1;
    move_intf = IMP_IVS_CreateMoveInterface(&move_param);

    ret = IMP_IVS_CreateChn(0, move_intf);
    if (ret < 0) {
        LOG_ERROR("IMP_IVS_CreateChn() == " << ret);
        return true;
    }

    ret = IMP_IVS_RegisterChn(0, 0);
    if (ret < 0) {
        LOG_ERROR("IMP_IVS_RegisterChn() == " << ret);
        return true;
    }

    ret = IMP_IVS_StartRecvPic(0);
    if (ret < 0) {
        LOG_ERROR("IMP_IVS_StartRecvPic() == " << ret);
        return true;
    }

    IMPCell fs = { DEV_ID_FS, 0, 0 };
    IMPCell ivs_cell = { DEV_ID_IVS, 0, 0 };
    //Framesource -> IVS
    ret = IMP_System_Bind(&fs, &ivs_cell);
    if (ret < 0) {
        LOG_ERROR("IMP_System_Bind(FS, IVS) == " << ret);
        return true;
    }
    return false;
}

void Motion::prebuffer(H264NALUnit &nal) {
    //Add the NALU to the end of the nalus list
    nalus.push_back(nal);
    //If it was a SPS nal, add a reference to the sps list
    if ((nal.data[0] & 0x1F) == 7) {
        sps.push_back(&nalus.back());
    }

    if (sps.size() > 1) {
        int del = 0;
        struct timeval now, diff;
        gettimeofday(&now, NULL);
        for (auto it = sps.begin(); it != sps.end(); ++it) {
            timersub(&now, &(*it)->time, &diff);
            if (diff.tv_sec >= MOTION_PRE_TIME) {
                ++del;
            }
        }

        //Pop H264NALUnits until we've deleted 'del' IDRs.
        while (del > 1) {
            nalus.pop_front();
            if ((nalus.front().data[0] & 0x1F) == 7) {
                //Pop off the SPS pointer
                sps.pop_front();
                --del;
            }
        }
    }
}

void Motion::run() {
    LOG_INFO("Starting Motion Detector");

    sink_id = Encoder::connect_sink(this);
    Motion::moving = false;
    detect_thread = std::thread(Motion::detect_start, this);

    H264NALUnit nal;
    while (true) {
        nal = encoder->wait_read();
        prebuffer(nal);

        if (clip != nullptr) {
            clip->write(nal);
        }
        if (!Motion::moving && clip != nullptr) {
            //End of motion clip
            clip->close();
            clip = nullptr;
        }
        else if (Motion::moving && clip == nullptr) {
            //Start recording
            clip = MotionClip::begin();
            //Flush the prebuffer into the clip
            auto last_idr = nalus.begin();
            for (auto it = nalus.begin(); it != nalus.end(); ++it) {
                clip->write(*it);
                if (((*it).data[0] & 0x1F) == 7) {
                    last_idr = it;
                }
            }
            //Clear all but the last IDR
            if (sps.size() > 1) {
                nalus.erase(nalus.begin(), last_idr);
                H264NALUnit* last_sps = sps.back();
                sps.clear();
                sps.push_back(last_sps);
            }
        }

#if MOTION_STRICT_IDR
        time_t cur = time(NULL);
        if (cur != last_time) {
            Encoder::flush();
            last_time = cur;
        }
#endif
    }

    return;
}
