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
#include "xrtcserver_def.hpp"
#include <memory>
#include "rtc_server.hpp"
#include "rtc_base/zmalloc.h"


extern xrtc::RtcServer* g_rtc_server;
namespace xrtc {
SignalingWorker::SignalingWorker(int worker_id, const signaling_server_conf& options)
    : worker_id_(worker_id), _options(options), event_loop_(new EventLoop(this)) {}

SignalingWorker::~SignalingWorker() {
    for (auto conn : conns_) {
        if (conn) {
            close_conn_(conn);
        }
    }
    conns_.clear();
    if (event_loop_) {
        delete event_loop_;
        event_loop_ = nullptr;
    }
    if (thread_) {
        delete thread_;
        thread_ = nullptr;
    }
}

void signaling_worker_recv_notify(EventLoop* /*el*/,
                                  IOWatcher* /*watcher*/,
                                  int fd,
                                  int /*events*/,
                                  void* data) {
    int msg;
    if (read(fd, &msg, sizeof(msg)) != sizeof(int)) {
        RTC_LOG(LS_WARNING) << "read notify from pipe failed:" << strerror(errno)
                            << ", error: " << errno;
        return;
    }
    RTC_LOG(LS_INFO) << "recv notify msg:" << msg;
    SignalingWorker* signaling_worker = static_cast<SignalingWorker*>(data);
    signaling_worker->process_notify(msg);
}

int SignalingWorker::init() {
    int fds[2];
    if (pipe(fds) < 0) {
        RTC_LOG(LS_WARNING) << "create pipe failed" << strerror(errno) << errno;
        return -1;
    }
    notify_recv_fd_ = fds[0];
    notify_send_fd_ = fds[1];
    pipe_watcher_   = event_loop_->creat_io_event(signaling_worker_recv_notify, this);
    event_loop_->start_io_event(pipe_watcher_, notify_recv_fd_, EventLoop::READ);
    return 0;
}

bool SignalingWorker::start() {
    if (thread_) {
        RTC_LOG(LS_WARNING) << "signaling worker is already running" << worker_id_;
        return false;
    }
    thread_ = new std::thread([=]() {
        RTC_LOG(LS_INFO) << "signaling worker start, worker id:" << worker_id_;
        event_loop_->start();
        RTC_LOG(LS_INFO) << "signaling worker stop, worker id:" << worker_id_;
    });
    return true;
}

void SignalingWorker::stop() {
    notify(SignalingWorker::QUIT);
}

int SignalingWorker::notify(int msg) {
    int written = write(notify_send_fd_, &msg, sizeof(int));
    return written == sizeof(msg) ? 0 : -1;
}

void SignalingWorker::process_notify(int msg) {
    switch (msg) {
        case QUIT:
            _stop();
            RTC_LOG(LS_INFO) << "signaling worker quit, worker id:" << worker_id_;
            break;
        case NEW_CONN:
            int fd;
            if (q_conn_.consume(&fd)) {
                new_conn(fd);
            }
            break;
        case RTC_MSG:
            _process_rtc_msg();
            break;
        default:
            RTC_LOG(LS_WARNING) << "unknow notify msg:" << msg;
            break;
    }
}


void SignalingWorker::_stop() {
    if (!thread_) {
        RTC_LOG(LS_WARNING) << "signaling worker is not running, worker id:" << worker_id_;
        return;
    }

    event_loop_->delete_io_event(pipe_watcher_);
    event_loop_->stop();

    close(notify_recv_fd_);
    close(notify_send_fd_);
}

void SignalingWorker::join() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

int SignalingWorker::notify_new_conn(int fd) {
    q_conn_.produce(fd);
    return notify(SignalingWorker::NEW_CONN);
}

void SignalingWorker::read_query(int fd) {
    RTC_LOG(LS_INFO) << "signaling worker " << worker_id_ << " read query from fd:" << fd;
    if (fd < 0 || (size_t)fd >= conns_.size() || !conns_[fd]) {
        RTC_LOG(LS_WARNING) << "invalid fd:" << fd;
        return;
    }
    TcpConnection* conn     = conns_[fd];
    int            nread    = 0;
    int            read_len = conn->bytes_expected;
    int            qb_len   = sdslen(conn->querybuf);

    conn->querybuf = sdsMakeRoomFor(conn->querybuf, read_len);
    nread          = sock_read_data(fd, conn->querybuf + qb_len, read_len);
    RTC_LOG(LS_INFO) << "sock read data, len: " << nread;

    conn->last_interaction_time = event_loop_->now();

    std::string data_read(conn->querybuf, sdslen(conn->querybuf));
    RTC_LOG(LS_INFO) << "Data read: " << data_read;

    if (nread == -1) {
        RTC_LOG(LS_WARNING) << "read query failed, fd:" << fd << ", error:" << strerror(errno)
                            << ", errno:" << errno;
        close_conn_(conn);
        return;
    } else if (nread > 0) {
        sdsIncrLen(conn->querybuf, nread);
    }
    RTC_LOG(LS_INFO) << "GOING TO PROCESS QUERY BUFFER";
    int ret = process_query_buffer(conn);
    if (ret < 0) {
        RTC_LOG(LS_WARNING) << "process query buffer failed, fd:" << fd;
        // close_conn_(conn);
        // delete conn;
        // conns_[fd]=nullptr;
        return;
    }
}

void SignalingWorker::_write_reply(int fd){
    if(fd<=0 || size_t(fd)>=conns_.size()){
        return;
    }
    TcpConnection* conn=conns_[fd];
    if(!conn){
        return;
    }
    while (!conn->reply_list.empty()) {
        rtc::Slice reply = conn->reply_list.front();
        int nwritten =sock_write_data(conn->fd_,reply.data()+ conn->cur_resp_pos,reply.size()-conn->cur_resp_pos);
        if(nwritten==-1){
            return;
        }else if(nwritten==0){
            RTC_LOG(LS_WARNING)<<"write zero bytes, fd: "<<conn->fd_<<", worker_id: "<<worker_id_;
        }else if((nwritten+conn->cur_resp_pos)>=reply.size()){
            //write success
            conn->reply_list.pop_front();
            zfree((void*)reply.data());
            conn->cur_resp_pos = 0;
            RTC_LOG(LS_INFO)<<"write finished, fd: "<<conn->fd_<<", worker_id: "<<worker_id_;
        }else{
            conn->cur_resp_pos += nwritten;
        }

    }
    conn->last_interaction_time=event_loop_->now();
    if(conn->reply_list.empty()){
        event_loop_->stop_io_event(conn->io_watcher_, conn->fd_,EventLoop::WRITE);
            RTC_LOG(LS_INFO)<<"write event stop, fd: "<<conn->fd_<<", worker_id: "<<worker_id_;

    }
}


void SignalingWorker::close_conn_(TcpConnection* conn) {
    close(conn->fd_);
    event_loop_->delete_timer(conn->timer_watcher_);
    event_loop_->delete_io_event(conn->io_watcher_);
    conns_[conn->fd_] = nullptr;
    delete conn;
}
void conn_io_cb(EventLoop* /*el*/, IOWatcher* /*watcher*/, int fd, int events, void* data) {
    SignalingWorker* worker = static_cast<SignalingWorker*>(data);
    if (events & EventLoop::READ) {
        worker->read_query(fd);
    }
    if (events & EventLoop::WRITE) {
        worker->_write_reply(fd);
    }
}


void conn_timer_cb(EventLoop* el, Timewatcher* /*watcher*/, void* data) {
    SignalingWorker* worker = (SignalingWorker*)el->owner();
    TcpConnection*   conn   = (TcpConnection*)data;
    worker->process_timeout(conn);
}

void SignalingWorker::process_timeout(TcpConnection* conn) {
    if (event_loop_->now() - conn->last_interaction_time >=
        (unsigned long)_options.connection_timeout) {
        RTC_LOG(LS_INFO) << "connection timeout, fd: " << conn->fd_;
        close_conn_(conn);
    }
}

void SignalingWorker::new_conn(int fd) {
    RTC_LOG(LS_INFO) << "new conn, worker id:" << worker_id_ << ", fd:" << fd;
    if (fd < 0) {
        RTC_LOG(LS_WARNING) << "invalid fd:" << fd;
        return;
    }

    sock_set_nonblock(fd);
    sock_set_nodelay(fd);

    TcpConnection* conn = new TcpConnection(fd);
    sock_peer_to_str(fd, conn->host_, conn->port_);

    conn->io_watcher_ = event_loop_->creat_io_event(conn_io_cb, this);
    event_loop_->start_io_event(conn->io_watcher_, fd, EventLoop::READ);

    conn->timer_watcher_ = event_loop_->creat_timer(conn_timer_cb, conn, 1);
    event_loop_->start_timer(conn->timer_watcher_, 100000); // 100ms

    conn->last_interaction_time = event_loop_->now();

    if ((size_t)fd >= conns_.size()) {
        conns_.resize(fd * 2, nullptr);
    }
    conns_[fd] = conn;
}

int SignalingWorker::process_request_(TcpConnection*    conn,
                                      const rtc::Slice& header,
                                      const rtc::Slice& body) {
    RTC_LOG(LS_INFO) << "receive body: " << body.data();
    xhead_t*                          xh = (xhead_t*)header.data();
    Json::CharReaderBuilder           builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value                       root;
    JSONCPP_STRING                    errs;
    reader->parse(body.data(), body.data() + body.size(), &root, &errs);
    if (!errs.empty()) {
        RTC_LOG(LS_WARNING) << "parse json body error, error:" << errs << ", fd " << conn->fd_
                            << ", log_id" << xh->log_id;
        return -1;
    }

    int cmdno;
    try {
        cmdno = root["cmdno"].asInt();

    } catch (Json::Exception e) {
        RTC_LOG(LS_WARNING) << "no cmdno field in body, log_id: " << xh->log_id;
        return -1;
    }

    switch (cmdno) {
        case CMDNO_PUSH:
            RTC_LOG(LS_INFO) << "CMDNO_PUSH";
            return process_push_(cmdno, conn, root, xh->log_id);
    }
    return 0;
}

int SignalingWorker::process_push_(int                cmdno,
                                   TcpConnection*     conn,
                                   const Json::Value& root,
                                   uint32_t           log_id) {
    uint64_t    uid;
    std::string stream_name;
    int         audio;
    int         video;

    try {
        uid         = root["uid"].asUInt64();
        stream_name = root["streamName"].asString();
        audio       = root["audio"].asInt();
        video       = root["video"].asInt();
    } catch (Json::Exception e) {
        RTC_LOG(LS_WARNING) << "parse push body error: " << e.what() << "log_id:" << log_id;
        return -1;
    }
    RTC_LOG(LS_INFO) << "cmdno: " << cmdno << " push stream, uid: " << uid
                     << ", stream_name: " << stream_name << ", audio: " << audio
                     << ", video: " << video << " signaling server push request";

    std::shared_ptr<RtcMsg> msg = std::make_shared<RtcMsg>();
    msg->cmdno                  = cmdno;
    msg->uid                    = uid;
    msg->stream_name            = stream_name;
    msg->audio                  = audio;
    msg->video                  = video;
    msg->log_id                 = log_id;
    msg->worker                 = this;
    msg->conn                   = conn;
    msg->fd                     = conn->fd_;
    return g_rtc_server->send_rtc_msg(msg);
}

int SignalingWorker::process_query_buffer(TcpConnection* conn) {
    while (sdslen(conn->querybuf) >= conn->bytes_expected + conn->bytes_processed) {
        RTC_LOG(LS_INFO) << "process query buffer, fd:" << conn->fd_;
        xhead_t* head = (xhead_t*)(conn->querybuf);
        if (conn->current_state == TcpConnection::STATE_HEAD) {
            RTC_LOG(LS_INFO) << "process head";
            if (head->magic_num != XHEAD_MAGIC) {
                RTC_LOG(LS_WARNING) << "invalid magic num:" << head->magic_num;
                return -1;
            }
            RTC_LOG(LS_INFO) << "process head success" << "head body len: " << head->body_len;
            conn->bytes_processed = XHEAD_SIZE;
            conn->bytes_expected  = head->body_len;
            conn->current_state   = TcpConnection::STATE_BODY;
        } else if (conn->current_state == TcpConnection::STATE_BODY) {
            RTC_LOG(LS_INFO) << "process body";
            rtc::Slice header(conn->querybuf, XHEAD_SIZE);
            rtc::Slice body(conn->querybuf + XHEAD_SIZE, head->body_len);

            int ret = process_request_(conn, header, body);
            if (ret < 0) {
                RTC_LOG(LS_WARNING) << "process request failed";
                return -1;
            }
            RTC_LOG(LS_INFO) << "process request success";
            // 短连接处理
            conn->bytes_processed = 65535;
        }
    }
    return 0;
}

void SignalingWorker::push_msg(std::shared_ptr<RtcMsg> msg) {
    std::unique_lock<std::mutex> lock(_q_msg_mtx);
    _q_msg.push(msg);
}
std::shared_ptr<RtcMsg> SignalingWorker::pop_msg() {
    std::unique_lock<std::mutex> lock(_q_msg_mtx);
    if (_q_msg.empty()) {
        return nullptr;
    }
    std::shared_ptr<RtcMsg> msg = _q_msg.front();
    _q_msg.pop();
    return msg;
}
int SignalingWorker::send_rtc_msg(std::shared_ptr<RtcMsg> msg) {
    push_msg(msg);
    return notify(RTC_MSG);
}

void SignalingWorker::_add_reply(TcpConnection* conn,const rtc::Slice& reply){
    conn->reply_list.emplace_back(reply);
    event_loop_->start_io_event(conn->io_watcher_, conn->fd_, EventLoop::WRITE);
}

void SignalingWorker::_process_rtc_msg() {
    std::shared_ptr<RtcMsg> msg = pop_msg();
    if (!msg) {
        return;
    }
    switch (msg->cmdno) {
        case CMDNO_PUSH:
            _response_server_offer(msg);
            break;
        default:
            RTC_LOG(LS_WARNING) << "unknown cmdno: " << msg->cmdno << ", log_id: " << msg->log_id;
            break;
    }
}

void SignalingWorker::_response_server_offer(std::shared_ptr<RtcMsg> msg) {
    TcpConnection* conn = static_cast<TcpConnection*>(msg->conn);
    if (!conn) {
        return;
    }

    int fd = msg->fd;
    if (fd < 0 || size_t(fd) >= conns_.size()) {
        return;
    }
    if (conns_[fd] != conn) {
        return;
    }

    // construct response header
    xhead_t*   xh = (xhead_t*)(conn->querybuf);
    rtc::Slice header(conn->querybuf, XHEAD_SIZE);
    char*      buf = (char*)zmalloc(XHEAD_SIZE + MAX_RES_BUF);
    if (!buf) {
        RTC_LOG(LS_WARNING) << "zmalloc error, log_id: " << xh->log_id;
        return;
    }

    memcpy(buf, header.data(), header.size());

    xhead_t*    res_xh = (xhead_t*)buf;
    Json::Value res_root;
    res_root["err_no"] = msg->err_no;
    if (msg->err_no != 0) {
        res_root["err_msg"] = "process error";
        res_root["offer"]   = "";
    } else {
        res_root["err_msg"] = "process success";
        res_root["offer"]   = msg->sdp;
    }

    Json::StreamWriterBuilder write_builder;
    write_builder.settings_["indentation"] = "";
    std::string json_data                  = Json::writeString(write_builder, res_root);
    RTC_LOG(LS_INFO) << "response body: " << json_data;

    res_xh->body_len = json_data.size();
    snprintf(buf + XHEAD_SIZE, MAX_RES_BUF, "%s", json_data.c_str());
    rtc::Slice reply(buf, XHEAD_SIZE + res_xh->body_len);
    _add_reply(conn,reply);
}


} // namespace xrtc
