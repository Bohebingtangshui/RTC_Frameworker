#include "signaling_worker.hpp"
#include "rtc_base/logging.h"
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <errno.h>
#include "base/Lock_Free_Queue.hpp"
#include "base/socket.hpp"
#include "tcp_connection.hpp"
namespace xrtc
{
SignalingWorker::SignalingWorker(int worker_id):worker_id_(worker_id),event_loop_(new EventLoop(this))
{

}

SignalingWorker::~SignalingWorker()
{
}

void signaling_worker_recv_notify(EventLoop* /*el*/, IOWatcher* /*watcher*/, int fd, int /*events*/, void* data)
{
    int msg;
    if(read(fd, &msg, sizeof(msg)) != sizeof(int))
    {
        RTC_LOG(LS_WARNING)<<"read notify from pipe failed:"<<strerror(errno)<<", error: "<<errno;
        return;
    }
    RTC_LOG(LS_INFO)<<"recv notify msg:"<<msg;
    SignalingWorker* signaling_worker = static_cast<SignalingWorker*>(data);
    signaling_worker->process_notify(msg);
}

int SignalingWorker::init(){
    int fds[2];
    if(pipe(fds) < 0)
    {
        RTC_LOG(LS_WARNING)<<"create pipe failed"<<strerror(errno)<<errno;
        return -1;
    }
    notify_recv_fd_ = fds[0];
    notify_send_fd_ = fds[1];
    pipe_watcher_ = event_loop_->creat_io_event(signaling_worker_recv_notify, this);
    event_loop_->start_io_event(pipe_watcher_, notify_recv_fd_, EventLoop::READ);
    return 0;
}

bool SignalingWorker::start()
{
    if(thread_)
    {
        RTC_LOG(LS_WARNING)<<"signaling worker is already running"<<worker_id_;
        return false;
    }
    thread_ = new std::thread([=](){
        RTC_LOG(LS_INFO)<<"signaling worker start, worker id:"<<worker_id_;
        event_loop_->start();
        RTC_LOG(LS_INFO)<<"signaling worker stop, worker id:"<<worker_id_;
    });
    return true;
} 

void SignalingWorker::stop()
{
    notify(SignalingWorker::QUIT);
}

int SignalingWorker::notify(int msg)
{
    int written = write(notify_send_fd_, &msg, sizeof(int));
    return written==sizeof(msg) ? 0 : -1;
}

void SignalingWorker::process_notify(int msg)
{
    switch (msg)
    {
        case QUIT:
            _stop();
            RTC_LOG(LS_INFO)<<"signaling worker quit, worker id:"<<worker_id_;
            break;
        case NEW_CONN:
            int fd;
            if(q_conn_.consume(&fd))
            {
                new_conn(fd);
            }
            break;
        default:
            RTC_LOG(LS_WARNING)<<"unknow notify msg:"<<msg;
            break;
    }
}

void SignalingWorker::_stop()
{
    if(!thread_){
        RTC_LOG(LS_WARNING)<<"signaling worker is not running, worker id:"<<worker_id_;
        return;
    }

    event_loop_->delete_io_event(pipe_watcher_);
    event_loop_->stop();

    close(notify_recv_fd_);
    close(notify_send_fd_);

}

void SignalingWorker::join()
{
    if(thread_ && thread_->joinable())
    {
        thread_->join();
    }
}  

int SignalingWorker::notify_new_conn(int fd)
{
    q_conn_.produce(fd);
    return notify(SignalingWorker::NEW_CONN);
}

void SignalingWorker::read_query(int fd){
    RTC_LOG(LS_INFO)<<"signaling worker "<<worker_id_<<" read query from fd:"<<fd ;

}

void conn_io_cb(EventLoop* /*el*/, IOWatcher* /*watcher*/, int fd, int events, void* data)
{
    SignalingWorker* worker = static_cast<SignalingWorker*>(data);
    if(events & EventLoop::READ)
    {
        worker->read_query(fd);
    }
}

void SignalingWorker::new_conn(int fd)
{   
    RTC_LOG(LS_INFO)<<"new conn, worker id:"<<worker_id_<<", fd:"<<fd;
    if(fd<0){
        RTC_LOG(LS_WARNING)<<"invalid fd:"<<fd;
        return;
    }

    sock_set_nonblock(fd);
    sock_set_nodelay(fd);

    TcpConnection* conn = new TcpConnection(fd);
    sock_peer_to_str(fd, conn->host_, conn->port_);

    conn->io_watcher_ = event_loop_->creat_io_event(conn_io_cb, this);
    event_loop_->start_io_event(conn->io_watcher_, fd, EventLoop::READ);

    if(fd>=conns_.size())
    {
        conns_.resize(fd+1, nullptr);
    }
    conns_[fd]=conn;

    
}

} // namespace xrtc
