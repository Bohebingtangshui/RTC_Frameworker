#pragma once 
#include <string>
#include "base/event_loop.hpp"
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
    };  
}