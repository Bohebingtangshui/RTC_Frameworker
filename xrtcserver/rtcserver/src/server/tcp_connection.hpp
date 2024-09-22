#pragma once
#include <string>
#include "base/event_loop.hpp"
#include "base/xhead.hpp"
#include "rtc_base/sds.h"
#include <list>
#include "rtc_base/slice.h"
namespace xrtc {
class TcpConnection {
public:
    enum {
        STATE_HEAD = 0,
        STATE_BODY = 1,
    };
    TcpConnection(int fd);
    ~TcpConnection();

public:
    int          fd_;
    std::string  host_;
    int          port_;
    IOWatcher*   io_watcher_{nullptr};
    Timewatcher* timer_watcher_{nullptr};

    sds    querybuf;
    size_t bytes_expected  = XHEAD_SIZE;
    size_t bytes_processed = 0;

    int                   current_state = STATE_HEAD;
    unsigned long         last_interaction_time;
    std::list<rtc::Slice> reply_list;          // 双向链表
    size_t                cur_resp_pos{0};
};
} // namespace xrtc