#include "SunTrack.hpp"

#define sind(x) ( sin(x * 3.14159265358979/180) )
#define cosd(x) ( cos(x * 3.14159265358979/180) )
#define asind(x) ( asin(x) / (3.14159265358979/180) )
#define acosd(x) ( acos(x) / (3.14159265358979/180) )

time_t SunTrack::julian2unix(double jd) {
    double out = jd;
    out -= 2440587.5;
    out *= 86400;
    return out;
}

double SunTrack::julianDate(time_t stamp) {
    double julian = stamp;
    julian /= 86400;
    julian += 2440587.5;
    return julian;
}

double SunTrack::julianDay(double jd) {
    double day = jd;
    day -= 2451545.0;
    day += 0.0008;
    return ceil(day);
}

double SunTrack::meanSolarTime(double day, double longitude) {
    double mst = day;
    mst -= (longitude / 360);
    return mst;
}

double SunTrack::meanSolarAnomaly(double mst) {
    double msa = 357.5291 + 0.98560028 * mst;
    return (int)msa % 360;
}

double SunTrack::equationOfCenter(double msa) {
    double c = 1.9148 * sind(msa);
    c += 0.0200 * sind(2 * msa);
    c += 0.0003 * sind(3 * msa);
    return c;
}

double SunTrack::eclipticLongitude(double msa, double c) {
    double lambda = msa + c + 180 + 102.9372;
    return (int)lambda % 360;
}

double SunTrack::solarTransit(double mst, double msa, double lambda) {
    double transit = 2451545.0 + mst + 0.0053 * sind(msa) - 0.0069 * sind(2 * lambda);
    return transit;
}

double SunTrack::solarDeclination(double lambda) {
    double declination = sind(lambda) * sind(23.44);
    return asind(declination);
}

double SunTrack::hourAngle(double declination, double latitude) {
    double costheta = (sind(-0.83) - sind(latitude) * sind(declination)) / (cosd(latitude) * cosd(declination));
    return acosd(costheta);
}

double SunTrack::julianSunrise(double transit, double theta) {
    return transit - (theta / 360);
}

double SunTrack::julianSunset(double transit, double theta) {
    return transit + (theta / 360);
}

int SunTrack::getSunInfo(double latitude, double longitude, time_t *rise, time_t *set) {
    time_t stamp = time(NULL);
    struct tm *bdt;
    bdt = localtime(&stamp);
    time_t utc_day = (stamp + bdt->tm_gmtoff) / 86400 * 86400;

    double n = julianDay(julianDate(utc_day));
    double mst = meanSolarTime(n, longitude);
    double msa = meanSolarAnomaly(mst);
    double c = equationOfCenter(msa);
    double lambda = eclipticLongitude(msa, c);
    double transit = solarTransit(mst, msa, lambda);
    double declination = solarDeclination(lambda);
    double theta = hourAngle(declination, latitude);
    double jrise = julianSunrise(transit, theta);
    double jset = julianSunset(transit, theta);

    if (std::isnan(jrise) || std::isnan(jset)) {
        return 1;
    }
    *rise = julian2unix(jrise);
    *set = julian2unix(jset);
    return 0;
}