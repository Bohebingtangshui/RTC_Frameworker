#pragma once
#include <string>
#include "base/event_loop.hpp"
#include <thread>
#include <vector>
namespace xrtc {
class SignalingWorker;
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
    void join();
    int notify(int msg);

    friend void signaling_server_recv_notify(EventLoop* el, IOWatcher* watcher, int fd, int events, void* data);
    friend void accept_new_conn(EventLoop * el, IOWatcher * watcher, int fd, int events, void * data);
private:
    void _process_notify(int msg); 
    void _stop();
    int create_worker(int worker_id);
    void dispatch_conn(int fd);

private:
    signaling_server_conf _options;
    EventLoop* event_loop_{nullptr};
    IOWatcher* io_watcher_{nullptr};
    std::thread* _thread{nullptr};

    IOWatcher* _pipe_watcher_{nullptr};
    int _notify_recv_fd{-1};
    int _notify_send_fd{-1};

    int _listen_fd{-1};

    std::vector<SignalingWorker*> workers_;

    int next_worker_{0};


};


}