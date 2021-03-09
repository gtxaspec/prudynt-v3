#ifndef MsgChannel_hpp
#define MsgChannel_hpp

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

template <class T> class MsgChannel {
public:
    MsgChannel(unsigned int bsize) : write_ptr(0), read_ptr(0) {
        msg_buffer.resize(bsize);
        buffer_size = bsize;
    }

    bool write(T msg) {
        if (can_write()) {
            std::unique_lock<std::mutex> lck(cv_mtx);
            msg_buffer[write_ptr] = msg;
            increment_write();
            write_cv.notify_all();
            return true;
        }
        else {
            return false;
        }
    }

    bool read(T *out) {
        if (can_read()) {
            *out = msg_buffer[read_ptr];
            msg_buffer[read_ptr] = T();
            increment_read();
            return true;
        }
        return false;
    }

    T wait_read() {
        std::unique_lock<std::mutex> lck(cv_mtx);
        while (!can_read()) {
            write_cv.wait(lck);
        };
        T val = msg_buffer[read_ptr];
        msg_buffer[read_ptr] = T();
        increment_read();
        return val;
    }

private:
    bool can_read() {
        //write pointer is never less than read pointer,
        //unless it has overflown and the read pointer hasn't.
        //read pointer cannot overflow before write pointer
        if (write_ptr < read_ptr) {
            //Write pointer overflow. We can read.
            return true;
        }
        else if (write_ptr > read_ptr) {
            return true;
        }
        else {
            return false;
        }
    }

    bool can_write() {
        if (((write_ptr + 1) % buffer_size) == read_ptr) {
            return false;
        }
        return true;
    }

    void increment_read() {
        read_ptr = (read_ptr + 1) % buffer_size;
    }

    void increment_write() {
        write_ptr = (write_ptr + 1) % buffer_size;
    }

    std::vector<T> msg_buffer;
    std::atomic<unsigned int> write_ptr;
    std::atomic<unsigned int> read_ptr;
    std::mutex cv_mtx;
    std::condition_variable write_cv;
    unsigned int buffer_size;
};

#endif
