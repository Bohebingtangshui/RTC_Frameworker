#pragma once

#include <string>
#include "libev/ev.h"
namespace xrtc {

class EventLoop;
class IOWatcher;
typedef void (*io_cb_t)(EventLoop* el, IOWatcher* watcher, int fd, int events,void* data);

class EventLoop {
public:
    enum{
        READ = 0x1,
        WRITE = 0x2,
    }
    EventLoop(void* owner);
    ~EventLoop();

    void start();
    void stop();

    IOWatcher* creat_io_event(io_cb_t cb, void* data);

private:
    struct ev_loop* loop_{nullptr};
    void* owner_{nullptr};

};
}