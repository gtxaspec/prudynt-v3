#include "Night.hpp"
#include "Config.hpp"
#include "GPIO.hpp"
#include "SunTrack.hpp"
#include "Logger.hpp"
#include <thread>
#include <imp/imp_framesource.h>
#include <imp/imp_system.h>
#include <imp/imp_encoder.h>
#include <imp/imp_isp.h>

#define MODULE "NIGHT"

void Night::init() {
    setDayMode(DAY_MODE_DAY);
}

void Night::update() {
    if (!Config::singleton()->nightEnabled)
        return;

    if (Config::singleton()->nightMode == NIGHT_MODE_SUN_TRACK) {
        if (time(NULL) - lastNightUpdate < 5*60)
            return;
        sunTrackUpdate();
    }
    lastNightUpdate = time(NULL);
}

void Night::sunTrackUpdate() {
    double lat = Config::singleton()->sunTrackLatitude;
    double lon = Config::singleton()->sunTrackLongitude;
    time_t rise, set;
    if (SunTrack::getSunInfo(lat, lon, &rise, &set) == 0) {
        LOG_INFO("Sunrise: " << rise << " Sunset: " << set);
        if (dayMode == DAY_MODE_DAY) {
            if (time(NULL) > set) {
                LOG_INFO("Switching to night mode after sunset.");
                setDayMode(DAY_MODE_NIGHT);
            }
        }
        else if (dayMode == DAY_MODE_NIGHT) {
            if (time(NULL) > rise && time(NULL) <= set) {
                LOG_INFO("Switching to day mode after sunrise.");
                setDayMode(DAY_MODE_DAY);
            }
        }
    }
}

void Night::setDayMode(DayMode mode) {
    dayMode = mode;
    if (mode == DAY_MODE_DAY) {
        //Day mode settings
        IMP_ISP_Tuning_SetISPRunningMode(IMPISP_RUNNING_MODE_DAY);

        //This may benefit from additional tuning
        //I found that 180 produces too much ghosting.
        //135 looks acceptable to my eye.
        IMP_ISP_Tuning_SetTemperStrength(128);
        IMP_ISP_Tuning_SetSinterStrength(135);

        //Enable IR filter
        std::thread filterThread([](){
            //Enable IR filter
            GPIO::write(53, 0);
            GPIO::write(52, 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            //Park motor
            GPIO::write(53, 1);
            GPIO::write(52, 1);
        });
        filterThread.detach();
        //Disable IR LEDs
        GPIO::write(49, 0);
        GPIO::write(26, 0);
    }
    else {
        //Night mode settings
        if (!Config::singleton()->nightColor) {
            IMP_ISP_Tuning_SetISPRunningMode(IMPISP_RUNNING_MODE_NIGHT);
        }

        if (Config::singleton()->nightInfrared) {
            //Enable IR LEDs
            GPIO::write(49, 1);
            GPIO::write(26, 1);
            std::thread filterThread([]() {
                //Disable IR filter
                GPIO::write(53, 1);
                GPIO::write(52, 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                //Park motor
                GPIO::write(53, 1);
                GPIO::write(52, 1);
            });
            filterThread.detach();
        }
    }
}
