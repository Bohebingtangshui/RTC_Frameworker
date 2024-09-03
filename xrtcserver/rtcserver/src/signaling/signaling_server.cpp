#include "signaling/signaling_server.hpp"
#include "yaml-cpp/yaml.h"
#include "rtc_base/logging.h"
#include "base/socket.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include "signaling_server.hpp"
#include <unistd.h>
#include "signaling_worker.hpp"


namespace xrtc {
    void signaling_server_recv_notify(EventLoop* /*el*/, IOWatcher* /*watcher*/, int fd, int /*events*/, void* data)
    {
        int msg;
        if(read(fd, &msg, sizeof(msg)) < 0)
        {
            RTC_LOG(LS_WARNING) << "read from pipe error: " << strerror(errno) << ", errno: " << errno;
            return;
        }
        RTC_LOG(LS_INFO)<<"recv notify msg:"<<msg;
        SignalingServer* signaling_server = static_cast<SignalingServer*>(data);
        signaling_server->_process_notify(msg);
    }

    void accept_new_conn(EventLoop * /*el*/, IOWatcher * /*watcher*/, int fd, int /*events*/, void * data)
    {
        int cfd;
        char cip[128];
        int cport;

        cfd=tcp_accept(fd, cip, &cport);
        if(cfd == -1)
        {
            RTC_LOG(LS_WARNING)<<"accept new conn failed";
            return;
        }
        RTC_LOG(LS_INFO)<<"accept new conn, ip:"<<cip<<", port:"<<cport;

        SignalingServer* signaling_server = static_cast<SignalingServer*>(data);
        signaling_server->dispatch_conn(cfd);
    }



    SignalingServer::SignalingServer():event_loop_(new EventLoop(this))
    {
    }

    SignalingServer::~SignalingServer()
    {
        if(event_loop_)
        {
            delete event_loop_;
            event_loop_ = nullptr;
        }
        if(_thread)
        {
            delete _thread;
            _thread = nullptr;
        }
        for(auto worker : workers_)
        {
            if(worker)
            {
                delete worker;
                worker = nullptr;
            }
        }
        workers_.clear();
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
            RTC_LOG(LS_INFO) << "signaling server options:\n" << conf;

            _options.host = conf["host"].as<std::string>();
            _options.port = conf["port"].as<int>();
            _options.worker_num = conf["worker_num"].as<int>();
            _options.connection_timeout = conf["connection_timeout"].as<int>();
            RTC_LOG(LS_INFO)<<"signaling server conf init success";
        }
        catch(YAML::Exception  e)
        {
            RTC_LOG(LS_WARNING) << "catch a YAML exception, line:" << e.mark.line + 1 << ", column: " << e.mark.column + 1 << ", error: " << e.msg;
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
        event_loop_->start_io_event(_pipe_watcher_, _notify_recv_fd, EventLoop::READ);

        _listen_fd = create_tcp_server(_options.host, _options.port);
        if(_listen_fd < 0)
        {
            RTC_LOG(LS_WARNING)<<"create tcp server failed";
            return -1;
        }
        io_watcher_ = event_loop_->creat_io_event(accept_new_conn,this);
        event_loop_->start_io_event(io_watcher_, _listen_fd, EventLoop::READ);

        // create worker
        for(int i = 0; i < _options.worker_num; ++i)
        {
            if(create_worker(i)!=0){
                RTC_LOG(LS_WARNING)<<"create worker failed";
                return -1;
            }
        }

        return 0;
    }

    bool SignalingServer::start()
    {
        if(_thread){
            RTC_LOG(LS_WARNING)<<"signaling server is already running";
            return false;
        }


        _thread = new std::thread([=](){
            RTC_LOG(LS_INFO)<<"signaling server event loop run";
            event_loop_->start();
            RTC_LOG(LS_INFO)<<"signaling server event loop stop";
        });
        return true;
    }

    void  SignalingServer::stop()
    {
        notify(SignalingServer::QUIT);
    }

    int SignalingServer::notify(int msg)
    {
        int written = write(_notify_send_fd, &msg, sizeof(msg));
        if(written < 0)
        {
            RTC_LOG(LS_WARNING)<<"write notify failed";
            return -1;
        }
        return written == sizeof(int) ? 0 : -1;
    }



    void SignalingServer::_process_notify(int msg)
    {
        switch (msg)
        {
            case QUIT:
                _stop();
                RTC_LOG(LS_INFO)<<"signaling server quit";
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
        RTC_LOG(LS_INFO)<<"signaling server delete_io_event";
        event_loop_->delete_io_event(io_watcher_);
        RTC_LOG(LS_INFO)<<"signaling server delete_io_event";
        event_loop_->stop();
        RTC_LOG(LS_INFO)<<"signaling server stop";
        close(_notify_recv_fd);
        close(_notify_send_fd);
        close(_listen_fd);

        RTC_LOG(LS_INFO)<<"signaling server stop";

        for(auto worker : workers_)
        {
            if (worker)
            {
                worker->stop();
                worker->join();
            }
        }
    }

    int SignalingServer::create_worker(int worker_id)
    {
        RTC_LOG(LS_INFO)<<"create worker:"<<worker_id;
        SignalingWorker* worker = new SignalingWorker(worker_id,_options);
        if(worker->init() != 0)
        {
            RTC_LOG(LS_WARNING)<<"worker init failed";
            return -1;
        }
        if(!worker->start())
        {
            RTC_LOG(LS_WARNING)<<"worker start failed";
            return -1;
        }
        workers_.emplace_back(worker);
        return 0;
    }

    void SignalingServer::join()
    {
        if(_thread)
        {
            RTC_LOG(LS_INFO)<<"signaling server thread true";
            if(_thread->joinable())
            {
                RTC_LOG(LS_INFO)<<"signaling server thread joinable";
                _thread->join();
            }
        }
    }

    void SignalingServer::dispatch_conn(int fd)
    {
        int index=next_worker_++;
        if(size_t(next_worker_)>=workers_.size())
        {
            next_worker_=0;
        }
        SignalingWorker* worker = workers_[index];
        worker->notify_new_conn(fd);       
    } 

}
