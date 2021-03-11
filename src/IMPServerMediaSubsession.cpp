#include <iostream>
#include "IMPServerMediaSubsession.hpp"
#include "IMPDeviceSource.hpp"
#include "H264VideoRTPSink.hh"
#include "H264VideoStreamDiscreteFramer.hh"
#include "GroupsockHelper.hh"

IMPServerMediaSubsession* IMPServerMediaSubsession::createNew(
    UsageEnvironment& env,
    StreamReplicator* rep,
    bool reuseFirstSource,
    H264NALUnit sps,
    H264NALUnit pps
) {
    return new IMPServerMediaSubsession(env, rep, reuseFirstSource, sps, pps);
}

IMPServerMediaSubsession::IMPServerMediaSubsession(
    UsageEnvironment& env,
    StreamReplicator* rep,
    bool reuseFirstSource,
    H264NALUnit sps,
    H264NALUnit pps)
    : OnDemandServerMediaSubsession(env, reuseFirstSource),
      replicator(rep), sps(sps), pps(pps)
{

}

IMPServerMediaSubsession::~IMPServerMediaSubsession() {

}

FramedSource* IMPServerMediaSubsession::createNewStreamSource(
    unsigned clientSessionId,
    unsigned& estBitrate
) {
    std::cout << "Create Stream Source." << std::endl;
    estBitrate = 5000;
    FramedSource *fs = replicator->createStreamReplica();

    return H264VideoStreamDiscreteFramer::createNew(envir(), fs, true, false);
}

RTPSink *IMPServerMediaSubsession::createNewRTPSink(
    Groupsock* rtpGroupsock,
    unsigned char rtpPayloadTypeIfDynamic,
    FramedSource* fs
) {
    increaseSendBufferTo(envir(), rtpGroupsock->socketNum(), 300*1024);
    return H264VideoRTPSink::createNew(
        envir(),
        rtpGroupsock,
        rtpPayloadTypeIfDynamic,
        &sps.data[0],
        sps.data.size(),
        &pps.data[0],
        pps.data.size()
    );
}
