#pragma once
#include <string>

namespace xrtc {
struct GeneralConf {
    std::string log_dir;
    std::string log_level;
    std::string log_name;
    bool log_to_stderr;
};

int Load_Genaral_Conf(const std::string& conf_file, GeneralConf& conf);

} 