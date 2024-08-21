#include "conf.hpp"
#include <iostream>
#include "yaml-cpp/yaml.h"
#include <string>
namespace xrtc {
int Load_Genaral_Conf(const std::string& conf_file, GeneralConf& conf) {
    if(conf_file.empty() || conf_file.size() > 1024) {
        std::cerr<<"file name is invalid"<<std::endl;
        return -1;
    }

    YAML::Node config= YAML::LoadFile(conf_file);
    try
    {
        conf.log_dir = config["log"]["log_dir"].as<std::string>();
        conf.log_name = config["log"]["log_name"].as<std::string>();
        conf.log_level = config["log"]["log_level"].as<std::string>();
        conf.log_to_stderr = config["log"]["log_to_stderr"].as<bool>();
    }
    catch(const YAML::Exception& e)
    {
        std::cerr << "catch a YAML::Exception, line:"<< e.mark.line<<"column:" <<e.mark.column<<"error:" << e.msg.c_str()<<'\n';
        return -1;
    }

    std::cout<<config<<std::endl;
    
    return 0;
}
}