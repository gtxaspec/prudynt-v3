#include "RTSP.hpp"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "IMPServerMediaSubsession.hpp"
#include "IMPDeviceSource.hpp"
#include "Config.hpp"

#define MODULE "RTSP"

void RTSP::run() {
    LOG_INFO("RUN");
    nice(-20);
    TaskScheduler *scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);

    RTSPServer *rtspServer;
    if (Config::singleton()->rtspAuthRequired) {
        UserAuthenticationDatabase *auth = new UserAuthenticationDatabase;
        auth->addUserRecord(
            Config::singleton()->rtspUsername.c_str(),
            Config::singleton()->rtspPassword.c_str()
        );
        rtspServer = RTSPServer::createNew(*env, 8554, auth);
    }
    else {
        rtspServer = RTSPServer::createNew(*env, 8554);
    }
    if (rtspServer == NULL) {
        LOG_ERROR("Failed to create RTSP server: " << env->getResultMsg() << "\n");
        return;
    }
    OutPacketBuffer::maxSize = 500000;

    int sink_id = Encoder::connect_sink(this, "SPSPPS");
    H264NALUnit sps, pps, vps;
    bool have_pps = false, have_sps = false, have_vps = false;
    //Read from the stream until we capture the SPS and PPS.
    while (!have_vps || !have_pps || !have_sps) {
        H264NALUnit unit = encoder->wait_read();
        uint8_t nalType = (unit.data[0] & 0x7E) >> 1;
        if (nalType == 33) {
            LOG_INFO("Got SPS");
            sps = unit;
            have_sps = true;
        }
        if (nalType == 34) {
            LOG_INFO("Got PPS");
            pps = unit;
            have_pps = true;
        }
        if (nalType == 32) {
            LOG_INFO("Got VPS");
            vps = unit;
            have_vps = true;
        }
    }
    Encoder::remove_sink(sink_id);
    LOG_INFO("Got VPS, PPS, & SPS.");

    ServerMediaSession *sms = ServerMediaSession::createNew(
        *env, "unicast", "Main", Config::singleton()->rtspName.c_str()
    );
    IMPServerMediaSubsession *sub = IMPServerMediaSubsession::createNew(
        *env, vps, sps, pps
    );
    sms->addSubsession(sub);
    rtspServer->addServerMediaSession(sms);

    char* url = rtspServer->rtspURL(sms);
    LOG_INFO("Play this stream from: " << url);

    env->taskScheduler().doEventLoop();
}
