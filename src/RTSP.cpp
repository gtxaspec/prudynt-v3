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

    int sink_id = Encoder::connect_sink(this);
    H264NALUnit sps, pps;
    bool have_pps = false, have_sps = false;
    //Read from the stream until we capture the SPS and PPS.
    while (!have_pps || !have_sps) {
        H264NALUnit unit = encoder->wait_read();
        if ((unit.data[0] & 0x1F) == 7) {
            sps = unit;
            have_sps = true;
        }
        if ((unit.data[0] & 0x1F) == 8) {
            pps = unit;
            have_pps = true;
        }
    }
    Encoder::remove_sink(sink_id);

    ServerMediaSession *sms = ServerMediaSession::createNew(
        *env, "Main", "Main", "Wyzecam"
    );
    IMPServerMediaSubsession *sub = IMPServerMediaSubsession::createNew(
        *env, sps, pps
    );
    sms->addSubsession(sub);
    rtspServer->addServerMediaSession(sms);

    char* url = rtspServer->rtspURL(sms);
    LOG(MODULE, "Play this stream from: " << url);

    env->taskScheduler().doEventLoop();
}
