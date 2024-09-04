#pragma once
#include "base/event_loop.hpp"
#include "server/tcp_connection.hpp"
#include <thread>
#include <vector>
#include "signaling_worker.hpp"
#include "xrtcserver_def.hpp"
#include <queue>
#include <mutex>
namespace xrtc {
    
    struct RtcServerOptions{
        int worker_num;
    };
    class RtcWorker;
    class RtcServer {
    public:
        enum{
            QUIT=0,
            RTC_MSG=1,
        };
        RtcServer();
        ~RtcServer();
        int Init(const std::string& conf_file);
        bool start();
        void stop();
        void join();
        int notify(int msg);
        int send_rtc_msg(std::shared_ptr<RtcMsg>);
        void push_msg(std::shared_ptr<RtcMsg>);
        std::shared_ptr<RtcMsg> pop_msg();
        friend void rtc_server_recv_notify(EventLoop*,IOWatcher*,int,int,void*);
    private:
        void process_notify(int msg);
        void _stop();
        void process_rtc_msg();
        int _create_worker(int worker_id);

        void new_conn(int fd);
        void read_query(int fd);
        void process_timeout(TcpConnection* conn);
        int process_push_(int cmdno,TcpConnection* conn,const Json::Value& root, uint32_t log_id);
    private:

        EventLoop* event_loop_;
        RtcServerOptions options_;
        IOWatcher* pipe_watcher_{nullptr};
        int notify_recv_fd_{-1};
        int notify_send_fd_{-1};
        std::thread* thread_{nullptr};
        std::queue<std::shared_ptr<RtcMsg>> _q_msg;
        std::mutex _q_msg_mtx;
        std::vector<RtcWorker*>_workers;
        std::vector<TcpConnection*>conns_;
    };
}