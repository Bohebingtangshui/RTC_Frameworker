#pragma once
#include "rtc_base/logging.h"
#include <string>
#include<fstream>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
namespace xrtc {

class XrtcLog :public rtc::LogSink
{
private:
    std::string log_dir;
    std::string log_level;
    std::string log_name;
    std::string log_out_file;
    std::string log_err_file;

    std::ofstream _out_file;
    std::ofstream _err_file;

    std::queue<std::string> _log_queue;
    std::mutex _log_mutex;

    std::queue<std::string> _err_queue;
    std::mutex _err_mutex;

    std::thread* _log_thread{nullptr};
    std::atomic<bool> _log_thread_running{false};


public:
XrtcLog(const std::string& log_dir, const std::string& log_level, const std::string& log_name);
~XrtcLog() override;

int InitLog();
bool start_log_thread();
void stop_log_thread();
void join_log_thread();
void set_log_to_stderr(bool log_to_stderr);
void OnLogMessage(const std::string& message, rtc::LoggingSeverity severity) override;
void OnLogMessage(const std::string& message) override;
};


}

