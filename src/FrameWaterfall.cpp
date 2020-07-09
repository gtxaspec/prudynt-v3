#include "FrameWaterfall.hpp"

#include <fstream>

#define MODULE "FrameWaterfall"

void FrameWaterfall::run() {
    std::fstream fs;
    fs.open("/tmp/h264.bin", std::fstream::out);
    while (true) {
        H264Frame frame = encoder->wait_read();
        fs.write((const char*)&frame.data[0], frame.data.size());
        LOG(MODULE, "ft " << frame.data.size());
    }
    fs.close();
}
