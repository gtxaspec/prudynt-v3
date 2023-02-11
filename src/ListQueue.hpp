#ifndef ListQueue_hpp
#define ListQueue_hpp

#include <atomic>
#include <iostream>
#include <condition_variable>
#include <mutex>

template <class T> struct ListQNode {
    T data;
    std::atomic<ListQNode*> next;
};

template <class T> class ListQueue {
public:
    ListQueue() {
        ListQNode<T> *qn = new ListQNode<T>;
        qn->next = NULL;
        head = qn;
        tail = qn;
        size = 0;
    }

    T wait_read() {
        std::unique_lock<std::mutex> lck(lq_mutex);
        if (head->next == NULL) {
            write_cv.wait(lck);
        }

        ListQNode<T> *phead = head;
        ListQNode<T> *pnext = head->next;
        if (pnext == NULL) {
            //Shouldn't be possible.
            std::cout << "No data in ListQueue after waiting on cv" << std::endl;
        }
        head = pnext;
        --size;
        delete phead;
        return pnext->data;
    }

    bool read(T* out) {
        std::unique_lock<std::mutex> lck(lq_mutex);
        ListQNode<T> *phead = head;
        ListQNode<T> *pnext = head->next;
        if (pnext == NULL) {
            return false;
        }
        *out = pnext->data;
        head = pnext;
        --size;
        delete phead;
        return true;
    }

    void write(T msg) {
        std::unique_lock<std::mutex> lck(lq_mutex);
        ListQNode<T> *add = new ListQNode<T>;
        add->data = msg;
        add->next = NULL;
        tail->next = add;
        tail = add;
        ++size;
        write_cv.notify_all();
    }

    int queue_size() {
        return size;
    }

private:
    ListQNode<T>* head;
    ListQNode<T>* tail;
    std::mutex lq_mutex;
    std::atomic<int> size;
    std::condition_variable write_cv;
};

#endif