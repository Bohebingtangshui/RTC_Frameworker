#pragma once
#include <string>
#include "base/event_loop.hpp"
#include <thread>
namespace xrtc {

struct signaling_server_conf
{
    std::string host;
    int port;
    int worker_num;
    int connection_timeout;
};

class SignalingServer {
public:
    enum  {
        QUIT = 0,
    };
    SignalingServer();
    ~SignalingServer();
    int Init(const std::string& conf_file);
    bool start();
    void stop();

    void _stop();

    void join();

    void notify(int msg);
friend void signaling_server_recv_notify(EventLoop* el, IOWatcher* watcher, int fd, int events, void* data);
private:
    int _process_notify(int msg); 

private:
    signaling_server_conf signaling_server_conf_;
    EventLoop* event_loop_{nullptr};
    IOWatcher* io_watcher_{nullptr};
    std::thread* _thread{nullptr};

    IOWatcher* _pipe_watcher_{nullptr};
    int _notify_recv_fd{-1};
    int _notify_send_fd{-1};

    int _listen_fd{-1};


};


}