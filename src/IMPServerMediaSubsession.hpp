#ifndef IMPServerMediaSubsession_hpp
#define IMPServerMediaSubsession_hpp

#include "StreamReplicator.hh"
#include "ServerMediaSession.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "Encoder.hpp"

class IMPServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
    static IMPServerMediaSubsession* createNew(
        UsageEnvironment& env,
        H264NALUnit vps,
        H264NALUnit sps,
        H264NALUnit pps
    );
protected:
    IMPServerMediaSubsession(
        UsageEnvironment& env,
        H264NALUnit vps,
        H264NALUnit sps,
        H264NALUnit pps
    );
    virtual ~IMPServerMediaSubsession();

protected:
    virtual FramedSource *createNewStreamSource(
        unsigned clientSessionId,
        unsigned &estBitrate
    );
    virtual RTPSink *createNewRTPSink(
        Groupsock *rtpGroupsock,
        unsigned char rtyPayloadTypeIfDynamic,
        FramedSource *inputSource
    );

private:
    StreamReplicator *replicator;
    H264NALUnit vps, sps, pps;
};

#endif
