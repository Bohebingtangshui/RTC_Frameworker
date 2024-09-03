#include "server/tcp_connection.hpp"

namespace xrtc {
    TcpConnection::TcpConnection(int fd):fd_(fd),querybuf(sdsempty())
    {
    }
    TcpConnection::~TcpConnection()
    {
        sdsfree(querybuf);
    }
}