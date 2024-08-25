#include "event_loop.hpp"
#include "libev/ev.h"
#include "rtc_base/logging.h"
#define TRANS_TO_EV_MASK(mask) \
    (((mask) & xrtc::EventLoopFlags::READ ? EV_READ : 0) | ((mask) & xrtc::EventLoopFlags::WRITE ? EV_WRITE : 0))

#define TRANS_FROM_EV_MASK(mask) \
    (((mask) & EV_READ ? xrtc::EventLoopFlags::READ : 0) | ((mask) & EV_WRITE ? xrtc::EventLoopFlags::WRITE : 0))
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
        RTC_LOG(LS_INFO)<<"event loop stop break";
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
    public: // Change the access specifier to public
        EventLoop* el_;
        // int fd_;
        // int events_;
        io_cb_t cb_;
        void* data_;
        struct ev_io io_;
    
    };

    static void generic_io_cb(struct ev_loop* loop, struct ev_io* w, int revents)
    {
        IOWatcher* watcher = static_cast<IOWatcher*>(w->data);
        watcher->cb_(watcher->el_, watcher, w->fd, TRANS_FROM_EV_MASK(revents), watcher->data_);
    }

    IOWatcher *EventLoop::creat_io_event(io_cb_t cb, void *data)
    {
        IOWatcher* watcher = new IOWatcher(this, cb, data);
        ev_init(&watcher->io_, generic_io_cb);

        return watcher;
    }

    void EventLoop::start_io_event(IOWatcher* watcher, int fd, int mask)
    {
        struct ev_io* io = &watcher->io_;
        if(ev_is_active(io))
        {
            int active_events = TRANS_FROM_EV_MASK(io->events);
            int new_events = active_events | mask;
            if(new_events == active_events)
            {
                return;
            }

            int events=TRANS_TO_EV_MASK(new_events);
            ev_io_stop(loop_, io);
            ev_io_set(io, fd, events);
            ev_io_start(loop_, io);

        }else{
            int events = TRANS_TO_EV_MASK(mask);
            ev_io_set(io, fd, events);
            ev_io_start(loop_, io);
        }
    }

    void EventLoop::stop_io_event(IOWatcher* watcher, int fd, int mask)
    {
        struct ev_io* io = &watcher->io_;
        int active_events = TRANS_FROM_EV_MASK(io->events);
        int new_events = active_events | ~mask;

        if(new_events == active_events)
        {
            return;
        }

        new_events = TRANS_TO_EV_MASK(new_events);
        ev_io_stop(loop_, io);

        if(new_events != EV_NONE)
        {
            ev_io_set(io, fd, new_events);
            ev_io_start(loop_, io);
        }

    }
    void EventLoop::delete_io_event(IOWatcher *watcher)
    {
        struct ev_io* io = &watcher->io_;
        ev_io_stop(loop_, io);
        delete watcher;
    }

    class Timewatcher
    {
    public:
        Timewatcher(EventLoop* el, time_cb_t cb, void* data, bool repeat):el_(el),cb_(cb),data_(data),repeat_(repeat)
        {
            timer_.data = this;
        }
        ~Timewatcher()
        {
        }

        EventLoop* el_;
        struct ev_timer timer_;
        time_cb_t cb_;
        void* data_;
        bool repeat_;

    };

    static void generic_time_cb(struct ev_loop* loop, struct ev_timer* timer, int revents)
    {
        Timewatcher* watcher = static_cast<Timewatcher*>(timer->data);
        watcher->cb_(watcher->el_, watcher, watcher->data_);
        if(!watcher->repeat_)
        {
            ev_timer_stop(loop, timer);
            delete watcher;
        }
    }

    xrtc::Timewatcher* EventLoop::creat_timer(xrtc::time_cb_t cb, void* data, bool repeat)
    {
        Timewatcher* watcher = new Timewatcher(this, cb, data, repeat);
        ev_init(&watcher->timer_, generic_time_cb);
        return watcher;
    }
    void EventLoop::start_timer(Timewatcher *watcher, unsigned int usec)
    {
        struct ev_timer* timer = &watcher->timer_;
        float sec = usec / 1000000.0;

        if(!timer->repeat)
        {
            ev_timer_set(timer, sec, 0);
            ev_timer_start(loop_, timer);
        }else{
            timer->repeat = sec;
            ev_timer_again(loop_, timer);
        }
    }
    void EventLoop::stop_timer(Timewatcher *watcher)
    {
        struct ev_timer* timer = &watcher->timer_;\
        ev_timer_stop(loop_, timer);
    }
    void EventLoop::delete_timer(Timewatcher *watcher)
    {
        stop_timer(watcher);
        delete watcher;
    }
}
