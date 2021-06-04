#include "MotionClip.hpp"
#include "IMP.hpp"

#include <libavutil/log.h>
#include <fstream>

extern "C" {
    #include <unistd.h>
}

void post_motion(std::string clip_path) {
    std::ifstream script_test("/home/wyze/scripts/post_motion", std::ifstream::in);
    if (!script_test.fail()) {
        char cmd[512];
        snprintf(cmd, 512, "/home/wyze/scripts/post_motion %s", clip_path.c_str());
        system(cmd);
        script_test.close();
    }
}

std::shared_ptr<MotionClip> MotionClip::begin() {
    return std::make_shared<MotionClip>(MotionClip());
}

MotionClip::MotionClip() {
    time_t now = time(NULL);
    struct tm *ltime = localtime(&now);
    char formatted[256];
    strftime(formatted, 256, "%Y-%m-%dT%H%M%S", ltime);
    formatted[255] = '\0';
    clip_timestr = std::string(formatted);

    clip_path = "/home/wyze/media/";
    clip_path += clip_timestr;
    clip_path += ".part.ts";

    LOG_DEBUG("Writing to " << clip_path);
    av_log_set_level(AV_LOG_DEBUG);
    avformat_alloc_output_context2(&oc, NULL, NULL, clip_path.c_str());
    if (!oc) {
        LOG_ERROR("Couldn't create output context");
        return;
    }

    oc->oformat->video_codec = AV_CODEC_ID_H264;
    vs = avformat_new_stream(oc, NULL);
    vs->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    vs->codecpar->codec_id = AV_CODEC_ID_H264;
    vs->codecpar->profile = FF_PROFILE_H264_HIGH;
    vs->codecpar->width = 1920;
    vs->codecpar->height = 1080;
    vs->codecpar->format = AV_PIX_FMT_YUV420P;
    vs->time_base.den = 900000;
    vs->time_base.num = 1;
    vs->avg_frame_rate.den = 1;
    vs->avg_frame_rate.num = IMP::FRAME_RATE;
    vs->r_frame_rate.den = 1;
    vs->r_frame_rate.num = IMP::FRAME_RATE;
    vs->pts_wrap_bits = 64;
    vs->start_time = AV_NOPTS_VALUE;

    if (avio_open(&oc->pb, clip_path.c_str(), AVIO_FLAG_WRITE) < 0) {
        LOG_ERROR("Could not open file");
        return;
    }
    if (avformat_write_header(oc, NULL) < 0) {
        LOG_ERROR("Unable to write clip header");
        return;
    }
    av_dump_format(oc, 0, clip_path.c_str(), 1);
}

void MotionClip::write(H264NALUnit nal) {
    int res;

    if (!init_pts_set) {
        initial_pts = nal.imp_ts;
        init_pts_set = true;
    }

    nal_buffer.push_back(nal);
    if ((nal.data[0] & 0x1F) == 5 || (nal.data[0] & 0x1F) == 1) {
        uint8_t startcode[4] = { 0, 0, 0, 1 };
        std::vector<uint8_t> naldata;
        int64_t nal_ts = 0;
        for (unsigned int i = 0; i < nal_buffer.size(); ++i) {
            naldata.insert(naldata.end(), startcode, startcode + 4);
            naldata.insert(
                naldata.end(),
                nal_buffer[i].data.begin(),
                nal_buffer[i].data.end()
            );
            nal_ts = nal_buffer[i].imp_ts;
        }
        nal_buffer.clear();

        AVPacket pkt;
        av_init_packet(&pkt);
        if ((nal.data[0] & 0x1F) == 5)
            pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = vs->index;
        pkt.data = &naldata[0];
        pkt.size = naldata.size();
        pkt.pts = nal_ts - initial_pts;
        pkt.dts = nal_ts - initial_pts;
        pkt.pos = -1;
        pkt.duration = 1000000 / IMP::FRAME_RATE;
        ++pts;
        av_packet_rescale_ts(&pkt, { 1, 1000000 }, vs->time_base);
        res = av_write_frame(oc, &pkt);
        if (res < 0) {
            LOG_ERROR("Error muxing packet: " << res);
        }
    }
}

void MotionClip::close() {
    av_write_trailer(oc);
    avio_closep(&oc->pb);
    avformat_free_context(oc);

    //Move motion clip to final & delete temp file
    std::ifstream tmpfile(clip_path, std::ifstream::in);
    std::string fin_path;
    fin_path = "/home/wyze/media/";
    fin_path += clip_timestr;
    fin_path += ".ts";
    std::ofstream finfile(fin_path, std::ofstream::out);
    finfile << tmpfile.rdbuf();
    finfile.close();
    std::remove(clip_path.c_str());

    //Execute post motion script
    std::thread script_exe(post_motion, fin_path);
    script_exe.detach();
}
