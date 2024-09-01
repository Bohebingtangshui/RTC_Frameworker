#include "signaling_worker.hpp"
#include "rtc_base/logging.h"
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <errno.h>
#include "base/Lock_Free_Queue.hpp"
#include "base/socket.hpp"
#include "tcp_connection.hpp"
#include "rtc_base/slice.h"
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
    if(fd<0 || (size_t)fd>=conns_.size() || !conns_[fd])
    {
        RTC_LOG(LS_WARNING)<<"invalid fd:"<<fd;
        return;
    }
    TcpConnection* conn = conns_[fd];
    int nread=0;
    int read_len=conn->bytes_expected;
    int qb_len=sdslen(conn->querybuf);

    conn->querybuf = sdsMakeRoomFor(conn->querybuf, read_len);
    nread = sock_read_data(fd,conn->querybuf+qb_len,read_len);
    RTC_LOG(LS_INFO)<< "sock read data, len: " << nread;

    std::string data_read(conn->querybuf, sdslen(conn->querybuf));
    RTC_LOG(LS_INFO) << "Data read: " << data_read;

    if(nread==-1)
    {
        RTC_LOG(LS_WARNING)<<"read query failed, fd:"<<fd<<", error:"<<strerror(errno)<<", errno:"<<errno;
        close_conn_(conn);
        return;
    }else if(nread>0){
        sdsIncrLen(conn->querybuf, nread);
    }
    RTC_LOG(LS_INFO)<<"GOING TO PROCESS QUERY BUFFER";
    int ret = process_query_buffer(conn);
    if(ret<0)
    {
        RTC_LOG(LS_WARNING)<<"process query buffer failed, fd:"<<fd;
        // close_conn_(conn);
        // delete conn;
        // conns_[fd]=nullptr;
        return;
    }
}

void SignalingWorker::close_conn_(TcpConnection* conn)
{
    close(conn->fd_);
    event_loop_->delete_io_event(conn->io_watcher_);
    conns_[conn->fd_]=nullptr;
    delete conn;
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

    if((size_t)fd>=conns_.size())
    {
        conns_.resize(fd*2, nullptr);
    }
    conns_[fd]=conn;

    
}

int SignalingWorker::process_request_(TcpConnection* conn, const rtc::Slice& header, const rtc::Slice& body)
{
    RTC_LOG(LS_INFO)<<"receive body: "<<body.data();
    return 0;
}

int SignalingWorker::process_query_buffer(TcpConnection* conn)
{
    while (sdslen(conn->querybuf) >= conn->bytes_expected+conn->bytes_processed)
    {
        RTC_LOG(LS_INFO)<<"process query buffer, fd:"<<conn->fd_;
        xhead_t* head = (xhead_t*)(conn->querybuf);
        if(conn->current_state == TcpConnection::STATE_HEAD)
        {
            RTC_LOG(LS_INFO)<<"process head";
            if(head->magic_num != XHEAD_MAGIC)
            {
                RTC_LOG(LS_WARNING)<<"invalid magic num:"<<head->magic_num;
                return -1;
            }
            RTC_LOG(LS_INFO)<<"process head success"<<"head body len: "<<head->body_len;
            conn->bytes_processed = XHEAD_SIZE;
            conn->bytes_expected = head->body_len;
            conn->current_state = TcpConnection::STATE_BODY;
        }else if(conn->current_state == TcpConnection::STATE_BODY)
        {
            RTC_LOG(LS_INFO)<<"process body";
            rtc::Slice header(conn->querybuf, XHEAD_SIZE);
            rtc::Slice body(conn->querybuf+XHEAD_SIZE, head->body_len);

            int ret=process_request_(conn, header, body);
            if(ret<0)
            {
                RTC_LOG(LS_WARNING)<<"process request failed";
                return -1;
            }
            RTC_LOG(LS_INFO)<<"process request success";
            //短连接处理
            conn->bytes_processed = 65535;

        }
    
    }
    return 0;
}

} // namespace xrtc
