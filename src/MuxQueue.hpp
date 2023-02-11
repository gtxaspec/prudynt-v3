#ifndef MUXQUEUE_HPP
#define MUXQUEUE_HPP

#include "ListQueue.hpp"
#include "MotionClip.hpp"

class MuxQueue {
public:
    MuxQueue();
    void run();

    void set_clip_source(std::shared_ptr<ListQueue<MotionClip*>> chn) {
        clip_source = chn;
    }
private:
    std::shared_ptr<ListQueue<MotionClip*>> clip_source;
    time_t queue_stats_time;
};

#endif