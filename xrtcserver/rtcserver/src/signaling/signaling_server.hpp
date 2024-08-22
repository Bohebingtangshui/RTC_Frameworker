#pragma once
#include <string>
namespace xrtc {

struct signaling_server_conf
{
    std::string host;
    int port;
    int worker_num;
    int connection_timeout;
};


class SignalingServer {
public:
    SignalingServer();
    ~SignalingServer();
    int Init(const std::string& conf_file);
private:
    signaling_server_conf signaling_server_conf_;
    
    int _listen_fd{-1};

};


}