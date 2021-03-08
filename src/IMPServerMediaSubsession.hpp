#ifndef IMPServerMediaSubsession_hpp
#define IMPServerMediaSubsession_hpp

#include "StreamReplicator.hh"
#include "ServerMediaSession.hh"
#include "OnDemandServerMediaSubsession.hh"

class IMPServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
    static IMPServerMediaSubsession* createNew(
        UsageEnvironment& env,
        StreamReplicator *rep,
        bool reuseFirstSource
    );
protected:
    IMPServerMediaSubsession(
        UsageEnvironment& env,
        StreamReplicator *rep,
        bool reuseFirstSource
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
};

#endif
