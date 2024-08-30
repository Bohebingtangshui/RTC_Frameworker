#include "signaling/tcp_connection.hpp"

namespace xrtc {
    TcpConnection::TcpConnection(int fd):fd_(fd),querybuf(sdsempty())
    {
    }
}