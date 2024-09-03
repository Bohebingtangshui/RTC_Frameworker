#pragma once

#include "libev/ev.h"

struct ev_loop;
namespace xrtc {

class EventLoop;
class IOWatcher;
class Timewatcher;
typedef void (*io_cb_t)(EventLoop* el, IOWatcher* watcher, int fd, int events,void* data);
typedef void (*time_cb_t)(EventLoop* el,Timewatcher* watcher, void* data);



class EventLoop {
public:
    enum  {
        READ = 0x1,
        WRITE = 0x2,
    };
    
    EventLoop(void* owner);
    ~EventLoop();

    void start();
    void stop();
    void* owner(){
        return owner_;
    }
    unsigned long now();

    IOWatcher* creat_io_event(io_cb_t cb, void* data);
    void start_io_event(IOWatcher* watcher, int fd, int mask);
    void stop_io_event(IOWatcher* watcher, int fd, int mask);
    void delete_io_event(IOWatcher* watcher);

    Timewatcher* creat_timer(time_cb_t cb, void* data,bool repeat);
    void start_timer(Timewatcher* watcher, unsigned int usec);
    void stop_timer(Timewatcher* watcher);
    void delete_timer(Timewatcher* watcher);

private:
    struct ev_loop* loop_{nullptr};
    void* owner_{nullptr};

};
}