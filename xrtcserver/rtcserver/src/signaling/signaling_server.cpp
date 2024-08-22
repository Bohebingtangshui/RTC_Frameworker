#include "signaling/signaling_server.hpp"
#include "yaml-cpp/yaml.h"
#include "rtc_base/logging.h"
#include "base/socket.hpp"
namespace xrtc {
    SignalingServer::SignalingServer()
    {
    }

    SignalingServer::~SignalingServer()
    {
    }
    int SignalingServer::Init(const std::string &conf_file)
    {
        if(conf_file.empty())
        {
            RTC_LOG(LS_WARNING)<<"conf file is empty";
            return -1;
        }
        try
        {
            YAML::Node conf = YAML::LoadFile(conf_file);
            signaling_server_conf_.host = conf["host"].as<std::string>();
            signaling_server_conf_.port = conf["port"].as<int>();
            signaling_server_conf_.worker_num = conf["worker_num"].as<int>();
            signaling_server_conf_.connection_timeout = conf["connection_timeout"].as<int>();
            RTC_LOG(LS_INFO)<<"signaling server conf init success";
        }
        catch(const std::exception& e)
        {
            RTC_LOG(LS_WARNING)<<"signaling server conf init failed";
            return -1;
        }

        _listen_fd = create_tcp_server(signaling_server_conf_.host, signaling_server_conf_.port);

        return 0;
    }
}
