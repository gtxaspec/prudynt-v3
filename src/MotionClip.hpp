#ifndef MotionClip_hpp
#define MotionClip_hpp

#include <fstream>
#include "Encoder.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}

class MotionClip {
public:
    static std::shared_ptr<MotionClip> begin();
    void write(H264NALUnit nal);
    void close();
private:
    MotionClip();
private:
    std::string clip_path;
    std::string clip_timestr;
    AVFormatContext* oc = NULL;
    AVStream* vs = NULL;
    std::vector<H264NALUnit> nal_buffer;
    int pts = 0;
    int64_t initial_pts;
    bool init_pts_set = false;
};

#endif
