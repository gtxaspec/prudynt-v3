#ifndef ListQueue_hpp
#define ListQueue_hpp

#include <atomic>
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
    }

    T wait_read() {
        ListQNode<T> *phead = head;
        ListQNode<T> *pnext;

        std::unique_lock<std::mutex> lck(cv_mtx);
        while ((pnext = phead->next) == NULL) {
            write_cv.wait(lck);
        }

        head = pnext;
        delete phead;
        return pnext->data;
    }

    bool read(T* out) {
        ListQNode<T> *phead = head;
        ListQNode<T> *pnext = head->next;
        if (pnext == NULL) {
            return false;
        }
        *out = pnext->data;
        head = pnext;
        delete phead;
        return true;
    }

    void write(T msg) {
        std::unique_lock<std::mutex> lck(cv_mtx);
        ListQNode<T> *add = new ListQNode<T>;
        add->data = msg;
        add->next = NULL;
        tail->next = add;
        tail = add;
        write_cv.notify_all();
    }

private:
    std::atomic<ListQNode<T>*> head;
    ListQNode<T>* tail;
    std::mutex cv_mtx;
    std::condition_variable write_cv;
};

#endif