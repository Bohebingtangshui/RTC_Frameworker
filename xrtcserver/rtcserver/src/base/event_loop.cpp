#include "event_loop.hpp"
#include "libev/ev.h"
#include "rtc_base/logging.h"

#define TRANS_TO_EV_MASK(mask) \
    (((mask) & xrtc::EventLoop::READ ? EV_READ : 0) | ((mask) & xrtc::EventLoop::WRITE ? EV_WRITE : 0))

#define TRANS_FROM_EV_MASK(mask) \
    (((mask) & EV_READ ? xrtc::EventLoop::READ : 0) | ((mask) & EV_WRITE ? xrtc::EventLoop::WRITE : 0))
// 将自定义事件与libev事件互相转换


namespace xrtc
{
    EventLoop::EventLoop(void *owner) : loop_(ev_loop_new(EVFLAG_AUTO)),
                                        owner_(owner)
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
        RTC_LOG(LS_INFO) << "event loop stop break";
        ev_break(loop_, EVBREAK_ALL);
    }
    unsigned long EventLoop::now()   // 获取事件循环中当前时间
    {
        return static_cast<unsigned long>(ev_now(loop_)*1000000);
    }

    class IOWatcher
    {
    public:
        IOWatcher(EventLoop *el, io_cb_t cb, void *data) : el_(el), cb_(cb), data_(data)
        {
            io_.data = this;
        }
        ~IOWatcher()
        {
        }

    public: // Change the access specifier to public
        EventLoop *el_;
        // int fd_;
        // int events_;
        io_cb_t cb_;
        void *data_;
        ev_io io_;    // ev_io中的watcher list维护了一个链表
    };

    static void generic_io_cb(struct ev_loop* /*loop*/, struct ev_io *w, int revents)
    {
        IOWatcher *watcher = static_cast<IOWatcher *>(w->data);
        watcher->cb_(watcher->el_, watcher, w->fd, TRANS_FROM_EV_MASK(revents), watcher->data_);
    } // 通用io回调函数

    IOWatcher *EventLoop::creat_io_event(io_cb_t cb, void *data)
    {
        IOWatcher *watcher = new IOWatcher(this, cb, data);
        ev_init(&(watcher->io_), generic_io_cb);

        return watcher;
    }   //  创建新的IOWatcher与初始化ev回调

    void EventLoop::start_io_event(IOWatcher *watcher, int fd, int mask)
    {
        struct ev_io *io = &(watcher->io_);
        if (ev_is_active(io))
        {
            int active_events = TRANS_FROM_EV_MASK(io->events);
            int new_events = active_events | mask;              // 将新事件与当前事件合并
            if (new_events == active_events)
            {
                return;
            }

            new_events = TRANS_TO_EV_MASK(new_events);   // 转换为外部掩码事件
            ev_io_stop(loop_, io);               // 重新设置新的掩码事件并重启
            ev_io_set(io, fd, new_events);
            ev_io_start(loop_, io);
        }
        else
        {
            int events = TRANS_TO_EV_MASK(mask);         //  io未激活，设置新的事件
            ev_io_set(io, fd, events);
            ev_io_start(loop_, io);
        }
    }

    void EventLoop::stop_io_event(IOWatcher *watcher, int fd, int mask)
    {
        struct ev_io *io = &(watcher->io_);
        int active_events = TRANS_FROM_EV_MASK(io->events);
        int new_events = active_events & ~mask;

        if (new_events == active_events)
        {
            return;
        }

        new_events = TRANS_TO_EV_MASK(new_events);
        ev_io_stop(loop_, io);

        if (new_events != EV_NONE)
        {
            ev_io_set(io, fd, new_events);
            ev_io_start(loop_, io);
        }
    }
    void EventLoop::delete_io_event(IOWatcher *watcher)
    {
        struct ev_io *io = &(watcher->io_);
        ev_io_stop(loop_, io);
        delete watcher;
    }

    class Timewatcher
    {
    public:
        Timewatcher(EventLoop *el, time_cb_t cb, void *data, bool repeat) : el_(el), cb_(cb), data_(data), repeat_(repeat)
        {
            timer_.data = this;
        }

        EventLoop *el_;
        struct ev_timer timer_;
        time_cb_t cb_;
        void *data_;
        bool repeat_;
    };

    static void generic_time_cb(struct ev_loop *loop, struct ev_timer *timer, int revents)
    {
        Timewatcher *watcher = static_cast<Timewatcher *>(timer->data);
        watcher->cb_(watcher->el_, watcher, watcher->data_);
    }

    xrtc::Timewatcher *EventLoop::creat_timer(xrtc::time_cb_t cb, void *data, bool repeat)
    {
        Timewatcher *watcher = new Timewatcher(this, cb, data, repeat);
        ev_init(&(watcher->timer_), generic_time_cb);
        return watcher;
    }
    void EventLoop::start_timer(Timewatcher *watcher, unsigned int usec)
    {
        struct ev_timer *timer = &(watcher->timer_);
        float sec = float(usec) / 1000000;

        if (!watcher->repeat_)
        {
            ev_timer_stop(loop_, timer);
            ev_timer_set(timer, sec, 0);
            ev_timer_start(loop_, timer);
        }
        else
        {
            timer->repeat = sec;
            ev_timer_again(loop_, timer);
        }
    }
    void EventLoop::stop_timer(Timewatcher *watcher)
    {
        struct ev_timer *timer = &(watcher->timer_);
        ev_timer_stop(loop_, timer);
    }
    void EventLoop::delete_timer(Timewatcher *watcher)
    {
        stop_timer(watcher);
        delete watcher;
    }
}
