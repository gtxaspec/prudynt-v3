#include "CVR.hpp"

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
    H264NALUnit nal;
    while (true) {
        nal = encoder->wait_read();
        if (!cvr_stream.is_open()) {
            std::string path = get_cvr_path();
            cvr_stream.open(path);
            if (!cvr_stream.good()) {
                LOG_ERROR("Failed to open CVR output file");
                return;
            }
            cvr_start = time(NULL);
        }

        //Write nal to CVR file
        cvr_stream.write(startcode, 4);
        cvr_stream.write((char*)&nal.data[0], nal.data.size());
        if (!cvr_stream.good()) {
            LOG_ERROR("Failed to write NALU to CVR stream");
            cvr_stream.close();
        }
        //Cycle cvr files every hour.
        if ((time(NULL) - cvr_start) >= 60 * 60) {
            LOG_INFO("Cycling CVR file...");
            cvr_stream.close();
        }

        std::this_thread::yield();
    }

    return;
}

std::string CVR::get_cvr_path() {
    time_t now = time(NULL);
    struct tm *ltime = localtime(&now);
    char formatted[256];
    strftime(formatted, 256, "%Y-%m-%dT%H%M%S", ltime);
    formatted[255] = '\0';
    std::string timestr = std::string(formatted);
    std::string cvr_path = "/home/wyze/media/CVR-";
    cvr_path += timestr;
    cvr_path += ".h265";
    return cvr_path;
}
