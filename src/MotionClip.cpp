#include "MotionClip.hpp"

std::shared_ptr<MotionClip> MotionClip::begin() {
    return std::make_shared<MotionClip>(MotionClip());
}

MotionClip::MotionClip() {
    time_t now = time(NULL);
    struct tm *ltime = localtime(&now);
    char formatted[256];
    strftime(formatted, 256, "%Y-%m-%dT%H%M%S", ltime);
    formatted[255] = '\0';

    clip_path = "/home/wyze/";
    clip_path += std::string(formatted);
    clip_path += ".h264";

    clip.open(clip_path, std::ofstream::out);
}

void MotionClip::write(H264NALUnit nal) {
    char startcode[4] = { 0, 0, 0, 1 };
    clip.write(startcode, 4);
    clip.write((char*)&nal.data[0], nal.data.size());
}

void MotionClip::close() {
    clip.close();
}
