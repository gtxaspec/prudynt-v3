#include "MuxQueue.hpp"

extern "C" {
    #include <unistd.h>
}

#define MODULE "MUX_QUEUE"

void MuxQueue::run() {
    MotionClip *mq;

    //We don't want to spend anything other than
    //excess time on this thread. The encoder, motion,
    //and rtsp threads are a lot more important.
    //Current nice is -20, inherited from Motion.
    nice(19);

    while (true) {
        mq = clip_source->wait_read();
        mq->write();
        delete mq;
        std::this_thread::yield();
    }
}