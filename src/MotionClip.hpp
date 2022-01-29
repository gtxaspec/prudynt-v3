#ifndef MotionClip_hpp
#define MotionClip_hpp

#include <fstream>
#include "Encoder.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}

struct NalMetadata {
    uint32_t size;
    int64_t imp_ts;
} __attribute__((packed));

class MotionClip {
public:
    static MotionClip* begin();
    void add_nal(H264NALUnit nal);
    void write();
private:
    MotionClip();
private:
    FILE *clip_file;
    FILE *meta_file;
    std::string clip_path;
    std::string meta_path;
    std::string clip_timestamp;
    std::string clip_timestr;
    bool files_ok = false;
};

#endif
