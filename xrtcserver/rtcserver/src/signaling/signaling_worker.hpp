#pragma once
#include "base/event_loop.hpp"
#include <thread>
#include "base/Lock_Free_Queue.hpp"
namespace xrtc {   
    class SignalingWorker
    {
    public:
        enum  {
            QUIT = 0,
            NEW_CONN = 1,
        };
        SignalingWorker(int worker_id);
        ~SignalingWorker();
        int init();
        bool start();
        void stop();
        int notify(int msg);

        friend void signaling_worker_recv_notify(EventLoop* el, IOWatcher* watcher, int fd, int events, void* data);
        void join();
        int notify_new_conn(int fd);
        

    private:
        void process_notify(int msg);
        void _stop();
        void new_conn(int fd);


    private:
        int worker_id_;

        EventLoop* event_loop_;
        IOWatcher* pipe_watcher_{nullptr};
        int notify_recv_fd_{-1};
        int notify_send_fd_{-1};

        std::thread* thread_{nullptr};
        LockFreeQueue<int> q_conn_;
    };
}