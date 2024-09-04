#include "rtc_server.hpp"
#include"yaml-cpp/yaml.h"
#include "rtc_base/logging.h"
#include "unistd.h"
#include "rtc_worker.hpp"
#include "rtc_base/crc32.h"


namespace xrtc {

    void rtc_server_recv_notify(EventLoop* ,IOWatcher*,int fd ,int /*events*/ ,void* data){
        int msg;
        if(read(fd, &msg, sizeof(int))<0){
            RTC_LOG(LS_WARNING)<<"read from pipe errpr: "<<strerror(errno)<<", errno: "<<errno;
            return;
        }
        RtcServer* server=static_cast<RtcServer*>(data);
        server->process_notify(msg);
    }

    RtcServer::RtcServer():event_loop_(new EventLoop(this))
    {
        
    }
    RtcServer::~RtcServer(){
        if (event_loop_) {
            delete event_loop_;
            event_loop_=nullptr;
        }
        if(thread_){
            delete thread_;
            thread_=nullptr;
        }

        for(auto worker:_workers){
            if(worker){
                delete worker;
            }
            _workers.clear();
        }
    }
    int RtcServer::Init(const std::string& conf_file){
        if(conf_file.empty()){
            RTC_LOG(LS_WARNING)<<"conf_file is null";
        }
        try {
            YAML::Node config = YAML::LoadFile(conf_file);
            RTC_LOG(LS_INFO)<<"rtc server options:\n"<<config;
            options_.worker_num=config["worker_num"].as<int>();
        } catch (YAML::Exception e) {
            RTC_LOG(LS_WARNING)<<"rtc server load conf file error"<<e.msg;
            return -1;
        }

        int fds[2];
        if(pipe(fds)<0){
            RTC_LOG(LS_WARNING)<<"create pipe error: "<<strerror(errno)<<", errno"<<errno;
            return -1;
        }

        notify_recv_fd_=fds[0];
        notify_send_fd_=fds[1];

        pipe_watcher_=event_loop_->creat_io_event(rtc_server_recv_notify, this);
        event_loop_->start_io_event(pipe_watcher_, notify_recv_fd_, EventLoop::READ);

        for(int i=0;i<options_.worker_num;i++){
            if(_create_worker(i)!=0){
                return -1;
            }
        }

        return 0;
    }

    int RtcServer::_create_worker(int worker_id){
        RTC_LOG(LS_INFO)<<"rtc server create worker, worker id : "<<worker_id;
        RtcWorker* worker = new RtcWorker(worker_id,options_);

        if(worker->init()!=0){
            return -1;
        }
        if(!worker->start()){
            return -1;
        }
        _workers.emplace_back(worker);
        return 0;
    }
    
    void RtcServer::process_notify(int msg){
        switch (msg) {
            case QUIT:
                _stop();
                break;
            case RTC_MSG:
                process_rtc_msg();
                break;
            default:
                RTC_LOG(LS_WARNING)<<"unknown msg:"<<msg;
                break;
        }
    }
    bool RtcServer::start(){
        if(thread_){
            RTC_LOG(LS_WARNING)<<"rtc server already start";
            return false;
        }
        thread_=new std::thread([=](){
            RTC_LOG(LS_INFO)<<"rtc server event loop start";
            event_loop_->start();
            RTC_LOG(LS_INFO)<<"rtc server event loop stop";
        });
        return true;
    }

    void RtcServer::stop(){
        notify(RtcServer::QUIT);
    }

    int RtcServer::notify(int msg){
        int written=write(notify_send_fd_, &msg, sizeof(msg));
        return written==sizeof(int) ? 0 : -1 ;
    }

    void RtcServer::_stop(){
        event_loop_->delete_io_event(pipe_watcher_);
        event_loop_->stop();
        close(notify_recv_fd_);
        close(notify_send_fd_);

        for(auto worker:_workers){
            if(worker){
                worker->stop();
                worker->join();
            }
        }

    }

    void RtcServer::join(){
        if(thread_ && thread_->joinable()){
            thread_->join();
        }
    }

    int RtcServer::send_rtc_msg(std::shared_ptr<RtcMsg> msg){
        push_msg(msg);
        return notify(RTC_MSG);
    }

    void RtcServer::push_msg(std::shared_ptr<RtcMsg> msg){
        std::unique_lock<std::mutex> lock(_q_msg_mtx);
        _q_msg.push(msg);
    }

    std::shared_ptr<RtcMsg> RtcServer::pop_msg(){
        std::unique_lock<std::mutex> lock(_q_msg_mtx);
        if(_q_msg.empty()){
            return nullptr;
        }
        std::shared_ptr<RtcMsg> msg=_q_msg.front();
        _q_msg.pop();
        return msg;
    }

    RtcWorker* RtcServer::get_worker(const std::string& stream_name){
        if(_workers.size()==0 || _workers.size()!=(size_t)options_.worker_num){
            return nullptr;
        }
        uint32_t num= rtc::ComputeCrc32(stream_name);
        size_t index = num % options_.worker_num;
        return _workers[index];
    }

    void RtcServer::process_rtc_msg(){
        std::shared_ptr<RtcMsg> msg=pop_msg();
        if(!msg){
            return;
        }

        RtcWorker* worker = get_worker(msg->stream_name);
        if(worker){
            worker->send_rtc_msg(msg);
        }
    }

}