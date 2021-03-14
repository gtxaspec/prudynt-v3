#ifndef IMPDeviceSource_hpp
#define IMPDeviceSource_hpp

#include "FramedSource.hh"
#include "MsgChannel.hpp"
#include "Encoder.hpp"

class IMPDeviceSource: public FramedSource {
public:
    static IMPDeviceSource* createNew(UsageEnvironment& env);

    void set_framesource(std::shared_ptr<MsgChannel<H264NALUnit>> chn) {
        encoder = chn;
    }

public:
    static EventTriggerId eventTriggerId;

protected:
    IMPDeviceSource(UsageEnvironment& env);
    virtual ~IMPDeviceSource();

private:
    virtual void doGetNextFrame();
    std::shared_ptr<MsgChannel<H264NALUnit>> encoder;
    uint32_t sink_id;
};

#endif
