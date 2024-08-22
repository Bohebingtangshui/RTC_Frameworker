#include <iostream>
#include "base/conf.hpp"
#include <memory>
#include "base/log.hpp"
#include "rtc_base/logging.h"
xrtc::GeneralConf* g_conf{nullptr};
xrtc::XrtcLog* g_log{nullptr};


int InitGeneralConf(const std::string& conf_file) {
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

int main() {
    if(InitGeneralConf("../rtcserver/conf/general.yaml") != 0) {
        return -1;
    }
    if(InitLog(g_conf->log_dir, g_conf->log_level, g_conf->log_name) != 0) {
        std::cerr<<"init log failed"<<std::endl;
        return -1;
    }
    std::cout<<"init log success"<<std::endl;
    g_log->set_log_to_stderr(g_conf->log_to_stderr);
    RTC_LOG(LS_VERBOSE)<<"hello world";


    g_log->join_log_thread();

    g_log->stop_log_thread();

    delete g_log;
    delete g_conf;
    return 0;
}