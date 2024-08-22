#include "socket.hpp"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "rtc_base/logging.h"

int xrtc::create_tcp_server(const std::string &host, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        RTC_LOG(LS_ERROR)<<"socket init failed";
        return -1;
    }

    int on=1;
    int ret= setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if(ret < 0) {
        RTC_LOG(LS_ERROR)<<"setsockopt failed";
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
    if(ret < 0) {
        RTC_LOG(LS_ERROR)<<"bind failed";
        close(sockfd);
        return -1;
    }

    ret= listen(sockfd, 4095);
    if(ret < 0) {
        RTC_LOG(LS_ERROR)<<"listen failed";
        close(sockfd);
        return -1;
    }

    return sockfd;
}