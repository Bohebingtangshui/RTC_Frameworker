#pragma once
#include <sys/socket.h>
#include <string>
namespace xrtc {

    int create_tcp_server(const std::string &host, int port);
    int tcp_accept(int fd, char* ip, int* port);
    int generic_accept(int fd, struct sockaddr* addr, socklen_t* len);
}