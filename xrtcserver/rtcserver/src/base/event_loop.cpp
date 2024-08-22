#include "event_loop.hpp"
#include "libev/ev.h"
#define TRANS_TO_EV_MASK(mask) \
    (((mask) & xrtc::EventLoop::READ ? EV_READ : 0) | ((mask) & xrtc::EventLoop::WRITE ? EV_WRITE : 0))

#define TRANS_FROM_EV_MASK(mask) \
    (((mask) & EV_READ ? xrtc::EventLoop::READ : 0) | ((mask) & EV_WRITE ? xrtc::EventLoop::WRITE : 0))
namespace xrtc {
    EventLoop::EventLoop(void* owner):loop_(ev_loop_new(EVFLAG_AUTO)),owner_(owner)
    { 
    }
    EventLoop::~EventLoop()
    {
    }
    void EventLoop::start()
    {
        ev_run(loop_);
    }
    void EventLoop::stop()
    {   
        ev_break(loop_, EVBREAK_ALL);
    }

    class IOWatcher
    {
    public:
        IOWatcher(EventLoop* el, io_cb_t cb, void* data):el_(el),cb_(cb),data_(data)
        {
            io_.data = this;
        }
        ~IOWatcher()
        {
        }
    private:
        EventLoop* el_;
        // int fd_;
        // int events_;
        io_cb_t cb_;
        void* data_;
        struct ev_io io_;

    };

    static void generic_io_cb(struct ev_io* io, int events)
    {
        IOWatcher* watcher = static_cast<IOWatcher*>(io->data);
        watcher->cb_(watcher->el_, watcher, io->fd, TRANS_FROM_EV_MASK(events), watcher->data_);
    }

    IOWatcher *EventLoop::creat_io_event(io_cb_t cb, void *data)
    {
        IOWatcher* watcher = new IOWatcher(this, cb, data);
        ev_init(&watcher->io_, generic_io_cb);

        return watcher;
    }
}
