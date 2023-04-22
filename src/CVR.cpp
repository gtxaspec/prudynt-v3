#include "CVR.hpp"
#include "MotionClip.hpp"

extern "C" {
    #include <unistd.h>
}

char startcode[] = { 0, 0, 0, 1 };

bool CVR::init() {
    return false;
}

void CVR::run() {
    LOG_INFO("Starting Continuous Recording");
    nice(-19);

    sink_id = Encoder::connect_sink(this, "CVR");

    time_t cvr_start = time(NULL);
    std::ofstream cvr_stream;
    std::ofstream meta_stream;
    H264NALUnit nal;
    while (true) {
        nal = encoder->wait_read();
        if (!cvr_stream.is_open()) {
            time_t now = time(NULL);
            std::string path = get_cvr_path(now, ".h265");
            std::string meta_path = get_cvr_path(now, ".meta");
            cvr_stream.open(path);
            meta_stream.open(path);
            if (!cvr_stream.good() || !meta_stream.good()) {
                LOG_ERROR("Failed to open CVR output file(s)");
                return;
            }
            cvr_start = now;
        }

        NalMetadata meta;
        meta.size = htole32(nal.data.size() + 4);
        meta.imp_ts = htole64(nal.imp_ts);
        //Write metadata to meta file
        meta_stream.write((uint8_t*)&meta, sizeof(NalMetadata));
        if (!meta_stream.good()) {
            LOG_ERROR("Failed to write NALU metadata");
            cvr_stream.close();
            meta_stream.close();
        }

        //Write nal to CVR file
        cvr_stream.write(startcode, 4);
        cvr_stream.write((char*)&nal.data[0], nal.data.size());
        if (!cvr_stream.good()) {
            LOG_ERROR("Failed to write NALU to CVR stream");
            cvr_stream.close();
            meta_stream.close();
        }

        //Cycle cvr files every hour.
        if ((time(NULL) - cvr_start) >= Config::singleton()->cvrRotateTime) {
            LOG_INFO("Cycling CVR file...");
            cvr_stream.close();
            meta_stream.close();
        }

        std::this_thread::yield();
    }

    return;
}

std::string CVR::get_cvr_path(time_t t, std::string ext) {
    struct tm *ltime = localtime(&t);
    char formatted[256];
    strftime(formatted, 256, "%Y-%m-%dT%H%M%S", ltime);
    formatted[255] = '\0';
    std::string timestr = std::string(formatted);
    std::string cvr_path = "/home/wyze/media/cvr/";
    cvr_path += timestr;
    cvr_path += ext;
    return cvr_path;
}