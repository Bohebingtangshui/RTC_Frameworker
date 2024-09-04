#include <iostream>
#include "base/conf.hpp"
#include <memory>
#include "base/log.hpp"
#include "rtc_base/logging.h"
#include "server/signaling_server.hpp"
#include <signal.h>
#include "server/rtc_server.hpp"

xrtc::GeneralConf* g_conf{nullptr};
xrtc::XrtcLog* g_log{nullptr};
xrtc::SignalingServer* g_signaling_server{nullptr};
xrtc::RtcServer* g_rtc_server{nullptr};
int InitGeneralConf(const std::string& conf_file) {
    if(conf_file.empty()) {
        std::cerr<<"conf file is empty"<<std::endl;
        return -1;
    }
    g_conf = new xrtc::GeneralConf();
    if(xrtc::Load_Genaral_Conf(conf_file, *g_conf) != 0) {
        std::cerr<< "load general conf failed"<<std::endl;
        return -1;
    }
    std::cout<<g_conf->log_level<<std::endl;
    return 0;
}

int InitLog(const std::string& log_dir, const std::string& log_level, const std::string& log_name) {
    g_log = new xrtc::XrtcLog(log_dir, log_level, log_name);
    if(g_log->InitLog() != 0) {
        std::cerr<<"init log failed"<<std::endl;
        return -1;
    }
    g_log->start_log_thread();
    return 0;
}

int InitSignalingServer(const std::string& conf_file) {
    g_signaling_server = new xrtc::SignalingServer();
    if(g_signaling_server->Init(conf_file) != 0) {
        std::cerr<<"init signaling server failed"<<std::endl;
        return -1;
    }
    return 0;
}

int InitRtcServer(const std::string& conf_file) {
    g_rtc_server = new xrtc::RtcServer();
    if(g_rtc_server->Init(conf_file) != 0) {
        std::cerr<<"init rtc server failed"<<std::endl;
        return -1;
    }
    return 0;
}

static void process_signal(int sig) {
    RTC_LOG(LS_INFO)<<"receive signal:"<<sig;
    if(SIGINT == sig || SIGTERM == sig) {
        if(g_signaling_server){
            RTC_LOG(LS_INFO)<<"signal stop signaling server";
            g_signaling_server->stop();
        }
        if(g_rtc_server){
            RTC_LOG(LS_INFO)<<"signal stop rtc server";
            g_rtc_server->stop();
        }
    }
}
int main() {
    if(InitGeneralConf("../conf/general.yaml") != 0) {
        return -1;
    }
    if(InitLog(g_conf->log_dir, g_conf->log_level, g_conf->log_name) != 0) {
        std::cerr<<"init log failed"<<std::endl;
        return -1;
    }

    std::cout<<"init log success"<<std::endl;
    g_log->set_log_to_stderr(g_conf->log_to_stderr);

    if(InitSignalingServer("../conf/signaling_server.yaml") != 0) {
        std::cerr<<"init signaling server failed"<<std::endl;
        return -1;
    }

    if(InitRtcServer("../conf/rtc_server.yaml") != 0) {
        std::cerr<<"init rtc server failed"<<std::endl;
        return -1;
    }

    signal(SIGINT,process_signal);
    signal(SIGTERM,process_signal);

    g_signaling_server->start();
    g_rtc_server->start();

    g_signaling_server->join();
    g_rtc_server->join();


    return 0;
}