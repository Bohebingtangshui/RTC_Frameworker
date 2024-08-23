#include "signaling/signaling_server.hpp"
#include "yaml-cpp/yaml.h"
#include "rtc_base/logging.h"
#include "base/socket.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include "signaling_server.hpp"
#include <unistd.h>

namespace xrtc {
    void signaling_server_recv_notify(EventLoop* el, IOWatcher* watcher, int fd, int events, void* data)
    {
        int msg;
        if(read(fd, &msg, sizeof(msg)) < 0)
        {
            RTC_LOG(LS_WARNING)<<"read notify from pipe failed";
            return;
        }
        RTC_LOG(LS_INFO)<<"recv notify msg:"<<msg;
        SignalingServer* signaling_server = static_cast<SignalingServer*>(data);
        signaling_server->_process_notify(msg);
    }
    void accept_new_conn(EventLoop* el, IOWatcher* watcher, int fd, int events, void* data)
    {
        SignalingServer* signaling_server = static_cast<SignalingServer*>(data);
        int conn_fd = accept(fd, nullptr, nullptr);
        if(conn_fd < 0)
        {
            RTC_LOG(LS_WARNING)<<"accept new conn failed";
            return;
        }
        RTC_LOG(LS_INFO)<<"accept new conn success";
    }

    SignalingServer::SignalingServer():event_loop_(new EventLoop(this))
    {
    }

    SignalingServer::~SignalingServer()
    {
    }
    int SignalingServer::Init(const std::string &conf_file)
    {
        if(conf_file.empty())
        {
            RTC_LOG(LS_WARNING)<<"conf file is empty";
            return -1;
        }
        try
        {
            YAML::Node conf = YAML::LoadFile(conf_file);
            signaling_server_conf_.host = conf["host"].as<std::string>();
            signaling_server_conf_.port = conf["port"].as<int>();
            signaling_server_conf_.worker_num = conf["worker_num"].as<int>();
            signaling_server_conf_.connection_timeout = conf["connection_timeout"].as<int>();
            RTC_LOG(LS_INFO)<<"signaling server conf init success";
        }
        catch(const std::exception& e)
        {
            RTC_LOG(LS_WARNING)<<"signaling server conf init failed";
            return -1;
        }

        // create pipe for notify
        int fds[2];
        if(pipe(fds) < 0)
        {
            RTC_LOG(LS_WARNING)<<"create pipe failed";
            return -1;
        }
        _notify_recv_fd = fds[0];
        _notify_send_fd = fds[1];
        //add recv fd to event loop
        _pipe_watcher_ = event_loop_->creat_io_event(signaling_server_recv_notify, this);
        event_loop_->start_io_event(_pipe_watcher_, _notify_recv_fd, EventLoopFlags::READ);

        _listen_fd = create_tcp_server(signaling_server_conf_.host, signaling_server_conf_.port);
        io_watcher_ = event_loop_->creat_io_event(accept_new_conn,this);
        event_loop_->start_io_event(io_watcher_, _listen_fd, EventLoopFlags::READ);
        return 0;
    }

    bool SignalingServer::start()
    {
        if(_thread){
            RTC_LOG(LS_WARNING)<<"signaling server is already running";
            return false;
        }


        _thread = new std::thread([this](){
            RTC_LOG(LS_INFO)<<"signaling server start";
            event_loop_->start();
            RTC_LOG(LS_INFO)<<"signaling server stop";
        });
        return true;
    }

    void  SignalingServer::stop()
    {
        notify(SignalingServer::QUIT);
    }

    void SignalingServer::notify(int msg)
    {
        int written = write(_notify_send_fd, &msg, sizeof(msg));
        if(written < 0)
        {
            RTC_LOG(LS_WARNING)<<"write notify failed";
            return;
        }
    }

    int SignalingServer::_process_notify(int msg)
    {
        switch (msg)
        {
            case SignalingServer::QUIT:
                _stop();
                break;
            default:
                RTC_LOG(LS_WARNING)<<"unknow notify msg:"<<msg;
                break;
        }
    }

    void SignalingServer::_stop()
    {
        if(!_thread)
        {
            RTC_LOG(LS_WARNING)<<"signaling server is not running";
            return;
        }
        event_loop_->delete_io_event(_pipe_watcher_);
        event_loop_->delete_io_event(io_watcher_);
        event_loop_->stop();
        close(_notify_recv_fd);
        close(_notify_send_fd);
        close(_listen_fd);

        RTC_LOG(LS_INFO)<<"signaling server stop";
    }   

    void SignalingServer::join()
    {
        if(_thread)
        {
            if(_thread->joinable())
            {
                _thread->join();
            }
        }
    }   
}
