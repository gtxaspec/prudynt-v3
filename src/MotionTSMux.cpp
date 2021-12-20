#include "MotionTSMux.hpp"
#include "Motion.hpp"
#include "IMP.hpp"
#include "Logger.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/log.h>
}

#include <fstream>

void MotionTSMux::mux(std::string timestr, std::string clip, std::string meta) {
    LOG_INFO("Muxing " << clip << " to MPEGTS.");

    AVFormatContext *oc = NULL;
    AVStream *vs = NULL;

    FILE *clip_file = fopen(clip.c_str(), "r");
    FILE *meta_file = fopen(meta.c_str(), "r");
    if (clip_file == NULL || meta_file == NULL) {
        LOG_ERROR("Can't mux clip, partial files don't exist.");
        return;
    }

    std::string tstmp_path = "/home/wyze/media/";
    tstmp_path += timestr;
    tstmp_path += ".part.ts";

    av_log_set_level(AV_LOG_DEBUG);
    avformat_alloc_output_context2(&oc, NULL, NULL, clip.c_str());
    if (!oc) {
        LOG_ERROR("Couldn't create output context");
        fclose(clip_file);
        fclose(meta_file);
        return;
    }

    oc->oformat->video_codec = AV_CODEC_ID_H265;
    vs = avformat_new_stream(oc, NULL);
    vs->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    vs->codecpar->codec_id = AV_CODEC_ID_H265;
    vs->codecpar->profile = FF_PROFILE_HEVC_MAIN;
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

    if (avio_open(&oc->pb, tstmp_path.c_str(), AVIO_FLAG_WRITE) < 0) {
        LOG_ERROR("Could not open output file");
        fclose(clip_file);
        fclose(meta_file);
        return;
    }
    if (avformat_write_header(oc, NULL) < 0) {
        LOG_ERROR("Unable to write clip header");
        fclose(clip_file);
        fclose(meta_file);
        return;
    }
    av_dump_format(oc, 0, tstmp_path.c_str(), 1);

    NalMetadata md;
    bool found_iframe = false;
    int64_t initial_pts = 0;
    std::vector<uint8_t> naldata;
    while (fread(&md, sizeof(NalMetadata), 1, meta_file) == 1) {
        md.size = le32toh(md.size);
        md.imp_ts = le64toh(md.imp_ts);

        uint8_t data[4 + md.size];
        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
        data[3] = 1;
        if (fread(&data[4], md.size, 1, clip_file) != 1) {
            LOG_ERROR("NAL data not readable from clip partial.");
            fclose(clip_file);
            fclose(meta_file);
            return;
        }
        naldata.insert(naldata.end(), data, data + md.size + 4);

        uint8_t nalType = (data[4] & 0x7E) >> 1;
        if (nalType == 32 && !found_iframe) {
            initial_pts = md.imp_ts;
            found_iframe = true;
        }

        if (nalType == 19 || nalType == 20 || nalType == 1) {
            AVPacket pkt;
            av_init_packet(&pkt);
            if (nalType == 19 || nalType == 20)
                pkt.flags |= AV_PKT_FLAG_KEY;
            pkt.stream_index = vs->index;
            pkt.data = naldata.data();
            pkt.size = naldata.size();
            pkt.pts = md.imp_ts - initial_pts;
            pkt.dts = md.imp_ts - initial_pts;
            pkt.pos = -1;
            pkt.duration = 1000000 / IMP::FRAME_RATE;
            av_packet_rescale_ts(&pkt, {1, 1000000}, vs->time_base);
            if (av_write_frame(oc, &pkt) < 0) {
                LOG_ERROR("Error muxing packet");
            }
            naldata.clear();
        }
    }
    if (!found_iframe) {
        LOG_ERROR("Clip contains no iframe! Something went wrong.");
    }

    fclose(clip_file);
    fclose(meta_file);
    av_write_trailer(oc);
    avio_closep(&oc->pb);
    avformat_free_context(oc);

    //Move motion clip to final & delete temp files
    std::ifstream tmpfile(tstmp_path, std::ifstream::in);
    std::string fin_path;
    fin_path = "/home/wyze/media/";
    fin_path += timestr;
    fin_path += ".ts";
    std::ofstream finfile(fin_path, std::ofstream::out);
    finfile << tmpfile.rdbuf();
    finfile.close();
    std::remove(tstmp_path.c_str());
    std::remove(clip.c_str());
    std::remove(meta.c_str());

    //Execute post motion script
}
