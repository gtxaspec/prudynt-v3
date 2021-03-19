#ifndef IMP_hpp
#define IMP_hpp

#include <imp/imp_framesource.h>
#include <imp/imp_system.h>
#include <imp/imp_isp.h>

class IMP {
public:
    static bool init();
public:
    static const int FRAME_RATE;
private:
    static int system_init();
    static int framesource_init();
    static IMPFSChnAttr create_fs_attr();
    static IMPSensorInfo create_sensor_info(std::string sensor);
};

#endif
