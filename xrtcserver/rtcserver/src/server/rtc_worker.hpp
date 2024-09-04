#pragma once
#include "rtc_server.hpp"
#include <thread>
namespace xrtc {
    class RtcWorker{
        public:
            enum{
                QUIT=0,
                RTC_MSG=1,
            };
            RtcWorker(int worker_id,const RtcServerOptions& options);
            ~RtcWorker();
            int init();
            bool start();
            void stop();
            int notify(int msg);
            void join();
            friend void rtc_worker_recv_notify(EventLoop* /*el*/, IOWatcher* /*w*/, int fd, int /*event*/, void* data);

        private:
            void _process_notify(int msg);
            void _stop();
        private:
            RtcServerOptions _options;
            int worker_id;
            EventLoop* _el;

            int _notify_recv_fd{-1};
            int _notify_send_fd{-1};

            IOWatcher* _pipe_watcher{nullptr};
            std::thread* _thread{nullptr};

    };
}