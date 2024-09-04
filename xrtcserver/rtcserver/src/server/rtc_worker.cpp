#include "rtc_worker.hpp"
#include "base/event_loop.hpp"
#include "rtc_base/logging.h"
#include <unistd.h>
namespace xrtc {
void rtc_worker_recv_notify(EventLoop*, IOWatcher*, int fd, int, void* data) {
    int msg;
    if (read(fd, &msg, sizeof(int)) != sizeof(int)) {
        RTC_LOG(LS_WARNING) << "read from pipe error: " << strerror(errno);
        return;
    }
    RtcWorker* worker = static_cast<RtcWorker*>(data);
    worker->_process_notify(msg);
}

RtcWorker::RtcWorker(int worker_id, const RtcServerOptions& options)
    : _options(options), worker_id(worker_id), _el(new EventLoop(this)) {}
RtcWorker::~RtcWorker() {
    if(_el){
        delete _el;
        _el=nullptr;
    }
    if(_thread){
        delete _thread;
        _thread=nullptr;
    }
}

int RtcWorker::init() {
    int fds[2];
    if (pipe(fds) < 0) {
        RTC_LOG(LS_WARNING) << " create pipe error: " << strerror(errno) << ", errno: " << errno;
        return -1;
    }

    _notify_recv_fd = fds[0];
    _notify_send_fd = fds[1];

    _pipe_watcher = _el->creat_io_event(rtc_worker_recv_notify, this);
    _el->start_io_event(_pipe_watcher, _notify_recv_fd, EventLoop::READ);
    return 0;
}


bool RtcWorker::start() {
    if(_thread){
        RTC_LOG(LS_WARNING)<<"rtc worker already start, worker_id: "<<worker_id;
        return false;
    }
    _thread=new std::thread([=](){
        RTC_LOG(LS_INFO)<<"rtc woker event loop start, worker id: "<<worker_id;
        _el->start();
        RTC_LOG(LS_INFO)<<"rtc woker event loop stop, worker id: "<<worker_id;
    });

    
    return true;
}

void RtcWorker::_process_notify(int msg) {
    switch (msg) {
        case QUIT:
            _stop();
            break;
        case RTC_MSG:
            process_rtc_msg();
            break;
        default:
            RTC_LOG(LS_WARNING)<<"unknown msg: "<<msg;
            break;

    }
}

void RtcWorker::_stop(){
    if(!_thread){
        RTC_LOG(LS_WARNING)<<"rtc worker not running, worker_id: "<<worker_id;
        return;
    }
    _el->delete_io_event(_pipe_watcher);
    _el->stop();
    close(_notify_recv_fd);
    close(_notify_send_fd);

}

void RtcWorker::stop(){
    notify(QUIT);
}

int RtcWorker::notify(int msg){
    int written = write(_notify_send_fd,&msg,sizeof(int));
    return written==sizeof(int) ? 0 : -1;
}

void RtcWorker::join(){
    if(_thread && _thread->joinable()){
        _thread->join();
    }
}

int RtcWorker::send_rtc_msg(std::shared_ptr<RtcMsg> msg){
    // add msg to queue belong to worker
    push_msg(msg);
    return notify(RTC_MSG);
}

void RtcWorker::push_msg(std::shared_ptr<RtcMsg> msg){
    _q_msg.produce(msg);
}

bool RtcWorker::pop_msg(std::shared_ptr<RtcMsg>* msg){
    return _q_msg.consume(msg);
}

void RtcWorker::process_rtc_msg(){
    std::shared_ptr<RtcMsg> msg;
    if(!pop_msg(&msg)){
        return;
    }
    RTC_LOG(LS_INFO)<<"cmdno["<<msg->cmdno<<"], uid["<<msg->uid
    <<"] streamName["<<msg->stream_name<<"] audio["<<msg->audio<<"] video["<<msg->video
    <<"] log_id:["<<msg->log_id<<"] rtc worker receive msg, worker id: "<<worker_id;

    switch (msg->cmdno) {
        case CMDNO_PUSH:
            _process_push(msg);
            break;
        default:
            RTC_LOG(LS_WARNING)<<"unknown cmdno: "<<msg->cmdno<<", log_id: "<<msg->log_id;
    }
}

void RtcWorker::_process_push(std::shared_ptr<RtcMsg> msg){
    
}

} // namespace xrtc