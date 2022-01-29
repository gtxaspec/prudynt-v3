#include "Scripts.hpp"
#include "Logger.hpp"

int Scripts::motionClip(std::string ts, std::string clip_path) {
    LOG_DEBUG("Executing motionClip script.");

    int ret;
    char formatted[512];
    memset(formatted, 0, 512);
    snprintf(
        formatted,
        512,
        "/home/wyze/scripts/motionClip %s %s",
        clip_path.c_str(),
        ts.c_str()
    );

    ret = system(formatted);
    if (ret != 0) {
        LOG_ERROR("motionClip script failed!");
    }
    return ret;
}
