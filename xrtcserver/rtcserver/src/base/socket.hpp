#pragma once

#include <string>
namespace xrtc {

    int create_tcp_server(const std::string &host, int port);
}