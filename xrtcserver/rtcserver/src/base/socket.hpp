#pragma once
#include <sys/socket.h>
#include <string>
namespace xrtc {

    int create_tcp_server(const std::string &host, int port);
    int tcp_accept(int fd, char* ip, int* port);
    int generic_accept(int fd, struct sockaddr* addr, socklen_t* len);
    int sock_set_nonblock(int fd);
    int sock_set_nodelay(int fd);
    int sock_peer_to_str(int fd, std::string& host, int& port);
    int sock_read_data(int fd, char* buf, int len);
}