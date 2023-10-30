#include "Config.hpp"
#include "Logger.hpp"
#include <libconfig.h++>

#define MODULE "CONFIG"

Config* Config::instance = nullptr;

Config::Config() {
    libconfig::Config lc;
    try {
        lc.readFile("/home/wyze/config/prudynt.cfg");
    }
    catch (...) {
        LOG_WARN("Failed to load prudynt configuration file");
        loadDefaults();
        return;
    }
    loadDefaults();
    lc.lookupValue("night.enabled", nightEnabled);
    lc.lookupValue("night.mode", nightModeString);
    lc.lookupValue("night.infrared", nightInfrared);
    lc.lookupValue("night.color", nightColor);
    lc.lookupValue("sun_track.latitude", sunTrackLatitude);
    lc.lookupValue("sun_track.longitude", sunTrackLongitude);
    lc.lookupValue("rtsp.auth_required", rtspAuthRequired);
    lc.lookupValue("rtsp.username", rtspUsername);
    lc.lookupValue("rtsp.password", rtspPassword);
    lc.lookupValue("rtsp.name", rtspName);
    lc.lookupValue("motion.enabled", motionEnabled);
    lc.lookupValue("motion.pre_time", motionPreTime);
    lc.lookupValue("motion.post_time", motionPostTime);
    lc.lookupValue("motion.sensitivity", motionSensitivity);
    lc.lookupValue("motion.debounce", motionDebounce);
    lc.lookupValue("motion.strict_idr", motionStrictIDR);
    lc.lookupValue("cvr.enabled", cvrEnabled);
    lc.lookupValue("cvr.rotate_time", cvrRotateTime);

    if (!validateConfig()) {
        LOG_ERROR("Configuration is invalid, using defaults.");
        loadDefaults();
        return;
    }
}

void Config::loadDefaults() {
    nightEnabled = true;
    nightInfrared = true;
    nightColor = false;
    nightModeString = "sun_track";
    nightMode = NIGHT_MODE_SUN_TRACK;
    sunTrackLatitude = 40.71;
    sunTrackLongitude = -74;
    rtspAuthRequired = false;
    rtspUsername = "";
    rtspPassword = "";
    rtspName = "Wyzecam";
    motionEnabled = true;
    motionPreTime = 5;
    motionPostTime = 5;
    motionSensitivity = 2;
    motionDebounce = 3;
    motionStrictIDR = false;
    cvrEnabled = false;
    cvrRotateTime = 3600;
}

bool Config::validateConfig() {
    if (nightModeString.compare("sun_track") != 0) {
        LOG_ERROR("The only supported night mode is sun_track.");
        return false;
    }
    if (sunTrackLatitude < -90 || sunTrackLatitude > 90) {
        LOG_ERROR("Sun track latitude out of range.");
        return false;
    }
    if (sunTrackLongitude < -180 || sunTrackLatitude > 180) {
        LOG_ERROR("Sun track longitude out of range.");
        return false;
    }
    if (motionSensitivity < 0 || motionSensitivity > 4) {
        LOG_ERROR("Motion sensitivity out of range.");
        return false;
    }
    if (motionPreTime < 0) {
        LOG_ERROR("Motion prebuffer time out of range.");
        return false;
    }
    if (motionPostTime < 0) {
        LOG_ERROR("Motion postbuffer time out of range.");
        return false;
    }
    if (motionDebounce < 1) {
        LOG_ERROR("Motion debounce must be at least one frame");
        return false;
    }
    if (cvrRotateTime <= 60) {
        LOG_ERROR("CVR rotation time out of range.");
        return false;
    }
    return true;
}

Config* Config::singleton() {
    if (Config::instance == nullptr) {
        Config::instance = new Config();
    }
    return Config::instance;
}
