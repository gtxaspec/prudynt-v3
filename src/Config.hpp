#pragma once

#include <string>

enum NightMode {
    NIGHT_MODE_SUN_TRACK
};

class Config {
public:
    static Config* singleton();
private:
    Config();
    void loadDefaults();
    bool validateConfig();

public:
    bool nightEnabled;
    bool nightInfrared;
    bool nightColor;
    std::string nightModeString;
    NightMode nightMode;
    double sunTrackLatitude;
    double sunTrackLongitude;
    std::string rtspUsername;
    std::string rtspPassword;
    std::string rtspName;
    bool rtspAuthRequired;
    bool motionEnabled;
    int motionPreTime;
    int motionPostTime;
    int motionSensitivity;
    int motionDebounce;
    bool motionStrictIDR;
    bool cvrEnabled;
    int cvrRotateTime;
private:
    static Config* instance;
};
