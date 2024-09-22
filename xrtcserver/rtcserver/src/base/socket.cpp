#include "socket.hpp"
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "rtc_base/logging.h"
#include <errno.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>


int xrtc::create_tcp_server(const std::string& host, int port) { 
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);    //创建服务器通讯描述符
    if (sockfd == -1) {
        RTC_LOG(LS_WARNING) << "socket init failed";
        return -1;
    }

    int on  = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));   // 设置使用等级，为所有协议SOL_SOCKET， 设置端口复用SO_REUSEADDR
    if (ret == -1) {
        RTC_LOG(LS_WARNING) << "setsockopt SO_REUSEADDR error, errno: " << errno
                            << ", error: " << strerror(errno);
        return -1;
    }

    struct sockaddr_in server_addr;    
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);     // 设置sa的端口和使用的ip，即本机所有ip地址

    if (!host.empty() && inet_aton(host.c_str(), &server_addr.sin_addr) == 0) {  //inet_aton将ip地址字符串转化为网络字节序
        RTC_LOG(LS_ERROR) << "inet_aton failed";
        close(sockfd);
        return -1;
    }

    ret = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)); //绑定套接字
    if (ret == -1) {
        RTC_LOG(LS_WARNING) << "bind error, errno: " << errno << ", error: " << strerror(errno);
        close(sockfd);
        return -1;
    }

    ret = listen(sockfd, 4095);   //监听套接字，最高容纳4096个链接处理请求
    if (ret == -1) {
        RTC_LOG(LS_WARNING) << "listen error, errno: " << errno << ", error: " << strerror(errno);
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int xrtc::tcp_accept(int fd, char* ip, int* port) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int cfd = generic_accept(fd, (struct sockaddr*)&client_addr, &len); // 获得需要连接的client fd
    if (cfd == -1) {
        RTC_LOG(LS_ERROR) << "accept failed";
        return -1;
    }
    if (ip) {
        strcpy(ip, inet_ntoa(client_addr.sin_addr));  //将网络字节序存储的ip转换为字符串赋值给ip
    }
    if (port) {
        *port = ntohs(client_addr.sin_port); // 将网络字节序存储的端口赋值给port
    }
    return cfd;
}

int xrtc::generic_accept(int fd, sockaddr* addr, socklen_t* len) {
    int cfd = -1;
    while (1) {
        cfd = accept(fd, addr, len);
        if (cfd == -1) {
            if (errno == EINTR) {   // 在非阻塞模式下，调用accept会直接返回错误值EINTR
            } else {
                RTC_LOG(LS_WARNING)
                    << "tcp accept error: " << strerror(errno) << ", errno: " << errno;
                return -1;
            }
        }
        break;
    }
    return cfd;
}

int xrtc::sock_set_nonblock(int fd) {   // 设置socket的非阻塞模式
    int flags = fcntl(fd, F_GETFL, 0);  // 获取fd当前文件状态标记
    if (flags == -1) {
        RTC_LOG(LS_WARNING) << "fcntl F_GETFL error, errno: " << errno
                            << ", error: " << strerror(errno);
        return -1;
    }
    flags |= O_NONBLOCK;   // O_NONBLOCK 标志位添加到现有标志中
    if (fcntl(fd, F_SETFL, flags) == -1) {
        RTC_LOG(LS_WARNING) << "fcntl F_SETFL error, errno: " << errno
                            << ", error: " << strerror(errno);
        return -1;
    }
    return 0;
}

int xrtc::sock_set_nodelay(int fd) {   // 禁用nagle算法，不使用小包堆积，防止粘包
    int flag = 1;
    int ret  = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    if (ret == -1) {
        RTC_LOG(LS_WARNING) << "setsockopt TCP_NODELAY error, errno: " << errno
                            << ", error: " << strerror(errno);
        return -1;
    }
    return 0;
}

int xrtc::sock_peer_to_str(int fd, std::string& host, int& port) {  // 服务端从fd中获取ip和port
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getpeername(fd, (struct sockaddr*)&addr, &len) == -1) {
        if (!host.empty()) {
            host.clear();
            host = "?";
        }
        if (port) {
            port = 0;
        }
        RTC_LOG(LS_WARNING) << "getpeername error, errno: " << errno
                            << ", error: " << strerror(errno);
        return -1;
    }
    host = inet_ntoa(addr.sin_addr);
    port = ntohs(addr.sin_port);

    return 0;
}

int xrtc::sock_read_data(int fd, char* buf, int len) {  // 从socket中读取数据
    int nread = read(fd, buf, len);
    if (nread == -1) {
        if (errno == EAGAIN) {   // 套接字为非阻塞模式，若无数据可读，会直接返回EAGAIN
            nread = 0;
        } else {
            RTC_LOG(LS_WARNING) << "sock read error, errno: " << errno
                                << ", error: " << strerror(errno);
        }
    } else if (nread == 0) {    // 返回值为0的时候，表示对端客户端已经关闭连接
        RTC_LOG(LS_WARNING) << "connection closed, fd: " << fd;
        return -1;
    }
    return nread;
}

int xrtc::sock_write_data(int fd, const char* buf, size_t len){
    int nwritten = write(fd, buf, len);
    if(nwritten==-1){
        if(EAGAIN == errno){    // 套接字为非阻塞模式，若无空间可写，会直接返回EAGAIN
            nwritten=0;
        }else{
            RTC_LOG(LS_WARNING)<<"sock write failed, error: "<<strerror(errno)<<", errno: "<<errno<<", fd: "<<fd;
            return -1;
        }
    }
    return nwritten;
}
