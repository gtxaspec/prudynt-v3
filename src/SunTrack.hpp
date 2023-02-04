#include <ctime>
#include <cmath>

class SunTrack {
public:
    static int getSunInfo(double latitude, double longitude, time_t *rise, time_t *set);
private:
    static time_t julian2unix(double jd);
    static double julianDate(time_t stamp);
    static double julianDay(double jd);
    static double meanSolarTime(double day, double longitude);
    static double meanSolarAnomaly(double mst);
    static double equationOfCenter(double msa);
    static double eclipticLongitude(double msa, double c);
    static double solarTransit(double mst, double msa, double lambda);
    static double solarDeclination(double lambda);
    static double hourAngle(double declination, double latitude);
    static double julianSunrise(double transit, double theta);
    static double julianSunset(double transit, double theta);
};