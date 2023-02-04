#pragma once

#include <ctime>

class Night {
public:
    void init();
    void update();
private:
    enum DayMode {
        DAY_MODE_DAY,
        DAY_MODE_NIGHT
    };

    void setDayMode(DayMode mode);
    void sunTrackUpdate();
private:
    DayMode dayMode;
    time_t lastNightUpdate;
};