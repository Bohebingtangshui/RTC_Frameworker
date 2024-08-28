#include "socket.hpp"
#include <iostream>
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


int xrtc::create_tcp_server(const std::string &host, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd ==-1) {
        RTC_LOG(LS_WARNING)<<"socket init failed";
        return -1;
    }

    int on=1;
    int ret= setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(ret ==-1) {
        RTC_LOG(LS_WARNING) << "setsockopt SO_REUSEADDR error, errno: " << errno
            << ", error: " << strerror(errno);
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(!host.empty() && inet_aton(host.c_str(), &server_addr.sin_addr) == 0) {
        RTC_LOG(LS_ERROR)<<"inet_aton failed";
        close(sockfd);
        return -1;
    }

    ret= bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret == -1) {
        RTC_LOG(LS_WARNING) << "bind error, errno: " << errno << ", error: " << strerror(errno);
        close(sockfd);
        return -1;
    }

    ret= listen(sockfd, 4095);
    if(ret == -1) {
        RTC_LOG(LS_WARNING) << "listen error, errno: " << errno << ", error: " << strerror(errno);
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int xrtc::tcp_accept(int fd, char *ip, int *port)
{
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int cfd = generic_accept(fd, (struct sockaddr*)&client_addr, &len);
    if(cfd==-1){
        RTC_LOG(LS_ERROR)<<"accept failed";
        return -1;
    }
    if(ip){
        strcpy(ip, inet_ntoa(client_addr.sin_addr));
    }
    if(port){
        *port = ntohs(client_addr.sin_port);
    }
    return cfd;
}

int xrtc::generic_accept(int fd, sockaddr *addr, socklen_t *len)
{
    int cfd=-1;
    while(1){
        cfd = accept(fd, addr, len);
        if(cfd==-1){
            if(errno==EINTR){
                continue;
            }else{
                RTC_LOG(LS_WARNING) << "tcp accept error: " << strerror(errno) << ", errno: " << errno;
                return -1;
            }

        }
        break;
    }
    return cfd;
}
