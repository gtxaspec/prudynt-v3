#ifndef IMPDeviceSource_hpp
#define IMPDeviceSource_hpp

#include "FramedSource.hh"
#include "MsgChannel.hpp"
#include "Encoder.hpp"

class IMPDeviceSource: public FramedSource {
public:
    static IMPDeviceSource* createNew(
        UsageEnvironment& env,
        std::shared_ptr<MsgChannel<H264NALUnit>> enc
    );

public:
    static EventTriggerId eventTriggerId;

protected:
    IMPDeviceSource(
        UsageEnvironment& env,
        std::shared_ptr<MsgChannel<H264NALUnit>> enc
    );
    virtual ~IMPDeviceSource();

private:
    virtual void doGetNextFrame();
    std::shared_ptr<MsgChannel<H264NALUnit>> encoder;
};

#endif
