#pragma once
#include "rtc_server.hpp"
#include <thread>
#include "xrtcserver_def.hpp"
#include "base/Lock_Free_Queue.hpp"
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
            void push_msg(std::shared_ptr<RtcMsg> msg);
            bool pop_msg(std::shared_ptr<RtcMsg>* msg);
            int send_rtc_msg(std::shared_ptr<RtcMsg> msg);
            friend void rtc_worker_recv_notify(EventLoop* /*el*/, IOWatcher* /*w*/, int fd, int /*event*/, void* data);

        private:
            void _process_notify(int msg);
            void _stop();
            void process_rtc_msg();
            void _process_push(std::shared_ptr<RtcMsg> msg);

        private:
            RtcServerOptions _options;
            int worker_id;
            EventLoop* _el;

            int _notify_recv_fd{-1};
            int _notify_send_fd{-1};

            IOWatcher* _pipe_watcher{nullptr};
            std::thread* _thread{nullptr};
            LockFreeQueue<std::shared_ptr<RtcMsg>>_q_msg;

    };
}