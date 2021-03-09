#include "RTSP.hpp"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "IMPServerMediaSubsession.hpp"
#include "IMPDeviceSource.hpp"

#define MODULE "RTSP"

void RTSP::run() {
    LOG(MODULE, "RUN");
    TaskScheduler *scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);

    RTSPServer *rtspServer = RTSPServer::createNew(*env, 8554, NULL);
    if (rtspServer == NULL) {
        LOG(MODULE, "Failed to create RTSP server: " << env->getResultMsg() << "\n");
        return;
    }
    OutPacketBuffer::maxSize = 500000;

    ServerMediaSession *sms = ServerMediaSession::createNew(
        *env, "Main", "Main", "Wyzecam"
    );
    IMPDeviceSource *imp = IMPDeviceSource::createNew(*env, encoder);
    StreamReplicator *replicator = StreamReplicator::createNew(*env, imp, false);
    IMPServerMediaSubsession *sub = IMPServerMediaSubsession::createNew(
        *env, replicator, true
    );
    sms->addSubsession(sub);
    rtspServer->addServerMediaSession(sms);

    char* url = rtspServer->rtspURL(sms);
    LOG(MODULE, "Play this stream from: " << url);

    env->taskScheduler().doEventLoop();
}
