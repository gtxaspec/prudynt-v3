#ifndef MotionClip_hpp
#define MotionClip_hpp

#include <fstream>
#include "Encoder.hpp"

class MotionClip {
public:
    static std::shared_ptr<MotionClip> begin();
    void write(H264NALUnit nal);
    void close();
private:
    MotionClip();
private:
    std::string clip_path;
    std::ofstream clip;
};

#endif
