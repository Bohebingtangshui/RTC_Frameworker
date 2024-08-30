#pragma once 
#include <string>
#include "base/event_loop.hpp"
#include "base/xhead.hpp"
#include "rtc_base/sds.h"
namespace xrtc {
    class TcpConnection {
    public:
        TcpConnection(int fd);
        ~TcpConnection();
        void Connect(const std::string &host, int port);
        void Send(const char* data, int len);
        void Close();

    public:
        int fd_;
        std::string host_;
        int port_;
        IOWatcher* io_watcher_{nullptr};

        sds querybuf;
        size_t bytes_expected=XHEAD_SIZE;
        size_t bytes_processed=0;
    };  
}