#include "IMPDeviceSource.hpp"
#include <iostream>
#include "GroupsockHelper.hh"

IMPDeviceSource* IMPDeviceSource::createNew(
    UsageEnvironment& env,
    std::shared_ptr<MsgChannel<H264NALUnit>> enc
) {
    return new IMPDeviceSource(env, enc);
}

IMPDeviceSource::IMPDeviceSource(
    UsageEnvironment& env,
    std::shared_ptr<MsgChannel<H264NALUnit>> enc)
    : FramedSource(env), encoder(enc)
{
    std::cout << "device source construct" << std::endl;
}

IMPDeviceSource::~IMPDeviceSource() {
    std::cout << "device source destruct" << std::endl;
}

void IMPDeviceSource::doGetNextFrame() {
    if (!isCurrentlyAwaitingData()) return;

    //XXX: We aren't supposed to block here. The correct thing would be
    //to return early if we don't have a frame available, and then signal
    //an event to deliver the frame when it becomes available.
    //See DeviceSource.cpp in 3rdparty/live/liveMedia/
    H264NALUnit nal = encoder->wait_read();

    if (nal.data.size() > fMaxSize) {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = nal.data.size() - fMaxSize;
    }
    else {
        fFrameSize = nal.data.size();
    }
    fPresentationTime = nal.time;
    memcpy(fTo, &nal.data[0], fFrameSize);

    FramedSource::afterGetting(this);
}
