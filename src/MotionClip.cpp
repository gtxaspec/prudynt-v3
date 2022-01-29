#include "MotionClip.hpp"
#include "IMP.hpp"
#include "MotionMP4Mux.hpp"

#include <fstream>

extern "C" {
    #include <unistd.h>
}

MotionClip* MotionClip::begin() {
    return new MotionClip();
}

MotionClip::MotionClip() {
    time_t now = time(NULL);
    struct tm *ltime = localtime(&now);
    char formatted[256];
    strftime(formatted, 256, "%Y-%m-%dT%H%M%S", ltime);
    formatted[255] = '\0';
    clip_timestr = std::string(formatted);
    strftime(formatted, 256, "%s", ltime);
    formatted[255] = '\0';
    clip_timestamp = std::string(formatted);

    clip_path = "/home/wyze/media/partial/";
    clip_path += clip_timestr;
    clip_path += ".part.h265";

    meta_path = "/home/wyze/media/partial/";
    meta_path += clip_timestr;
    meta_path += ".part.meta";

    LOG_DEBUG("Writing to " << clip_path);

    clip_file = fopen(clip_path.c_str(), "w");
    if (clip_file == NULL) {
        LOG_ERROR("Could not open nal file");
        return;
    }

    meta_file = fopen(meta_path.c_str(), "w");
    if (meta_file == NULL) {
        LOG_ERROR("Could not open metadata file");
        return;
    }

    files_ok = true;
}

void MotionClip::add_nal(H264NALUnit nal) {
    NalMetadata m;
    m.size = htole32(nal.data.size());
    m.imp_ts = htole64(nal.imp_ts);

    if (!files_ok) {
        LOG_ERROR("Files are not open!");
        return;
    }

    if (fwrite(&nal.data[0], nal.data.size(), 1, clip_file) != 1) {
        LOG_ERROR("Failed to write NAL unit!");
    }
    if (fwrite(&m, sizeof(NalMetadata), 1, meta_file) != 1) {
        LOG_ERROR("Failed to write NAL metadata!");
    }
    //could fsync here?
}

void MotionClip::write() {
    if (!files_ok) {
        return;
    }
    fsync(fileno(clip_file));
    fsync(fileno(meta_file));
    fclose(clip_file);
    fclose(meta_file);
    files_ok = false;

    //Move motion clip to final & delete temp file
    std::ifstream nal_tmpfile(clip_path, std::ifstream::in);
    std::string nal_fin_path;
    nal_fin_path = "/home/wyze/media/partial/";
    nal_fin_path += clip_timestr;
    nal_fin_path += ".h265";
    std::ofstream nal_finfile(nal_fin_path, std::ofstream::out);
    nal_finfile << nal_tmpfile.rdbuf();
    nal_finfile.close();
    nal_tmpfile.close();
    std::remove(clip_path.c_str());

    //Move motion clip to final & delete temp file
    std::ifstream meta_tmpfile(meta_path, std::ifstream::in);
    std::string meta_fin_path;
    meta_fin_path = "/home/wyze/media/partial/";
    meta_fin_path += clip_timestr;
    meta_fin_path += ".meta";
    std::ofstream meta_finfile(meta_fin_path, std::ofstream::out);
    meta_finfile << meta_tmpfile.rdbuf();
    meta_finfile.close();
    meta_tmpfile.close();
    std::remove(meta_path.c_str());

    MotionMP4Mux::mux(clip_timestamp, clip_timestr, nal_fin_path, meta_fin_path);
}
