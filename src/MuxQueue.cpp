#include "MuxQueue.hpp"

extern "C" {
    #include <unistd.h>
}

#define MODULE "MUX_QUEUE"

MuxQueue::MuxQueue() {
    queue_stats_time = time(NULL);
}

void MuxQueue::run() {
    MotionClip *mq;

    //We don't want to spend anything other than
    //excess time on this thread. The encoder, motion,
    //and rtsp threads are a lot more important.
    //Current nice is -20, inherited from Motion.
    nice(19);

    while (true) {
        //Print queue statistics every 3 minutes
        time_t now = time(NULL);
        if (now - queue_stats_time > 3*60) {
            LOG_INFO("MuxQueue size: " << clip_source->queue_size());
            queue_stats_time = now;
        }

        mq = clip_source->wait_read();
        mq->write();
        delete mq;
        std::this_thread::yield();
    }
}