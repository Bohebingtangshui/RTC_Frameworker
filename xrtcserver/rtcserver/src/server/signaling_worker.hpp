#pragma once
#include "base/event_loop.hpp"
#include <thread>
#include "base/Lock_Free_Queue.hpp"
#include "tcp_connection.hpp"
#include <vector>
#include "rtc_base/slice.h"
#include "signaling_server.hpp"
#include "json/json.h"
#include "xrtcserver_def.hpp"
#include <queue>
#include <mutex>
namespace xrtc {   
    class TcpConnection;
    class SignalingWorker
    {
    public:
        enum  {
            QUIT = 0,
            NEW_CONN = 1,
            RTC_MSG=2,
        };
        SignalingWorker(int worker_id,const signaling_server_conf& options);
        ~SignalingWorker();
        int init();
        bool start();
        void stop();
        int notify(int msg);
        void push_msg(std::shared_ptr<RtcMsg> msg);
        std::shared_ptr<RtcMsg> pop_msg();
        int send_rtc_msg(std::shared_ptr<RtcMsg> msg);

        friend void signaling_worker_recv_notify(EventLoop* el, IOWatcher* watcher, int fd, int events, void* data);
        friend void conn_io_cb(EventLoop* el, IOWatcher* watcher, int fd, int events, void* data);
        void join();
        int notify_new_conn(int fd);
        int process_query_buffer(TcpConnection* conn);
        int process_request_(TcpConnection* conn, const rtc::Slice& header, const rtc::Slice& body);
        void close_conn_(TcpConnection* conn);
        void _process_rtc_msg();
        void _response_server_offer(std::shared_ptr<RtcMsg> msg);
        void _add_reply(TcpConnection* conn,const rtc::Slice& reply);
        void _write_reply(int fd);
        friend void conn_timer_cb(EventLoop* el, Timewatcher* /*watcher*/, void* data);

    private:
        void process_notify(int msg);
        void _stop();
        void new_conn(int fd);
        void read_query(int fd);
        // void _write_reply(int fd);
        void process_timeout(TcpConnection* conn);
        int process_push_(int cmdno,TcpConnection* conn,const Json::Value& root, uint32_t log_id);

    private:
        int worker_id_;
        signaling_server_conf _options;
        EventLoop* event_loop_;
        IOWatcher* pipe_watcher_{nullptr};
        int notify_recv_fd_{-1};
        int notify_send_fd_{-1};

        std::thread* thread_{nullptr};
        LockFreeQueue<int> q_conn_;

        std::vector<TcpConnection*>conns_;

        std::queue<std::shared_ptr<RtcMsg>> _q_msg;
        std::mutex _q_msg_mtx;
    };
}