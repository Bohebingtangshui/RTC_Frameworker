#include "base/log.hpp"
#include "log.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sys/stat.h>
#include <format>
#include <sstream>
namespace xrtc {


xrtc::XrtcLog::XrtcLog(const std::string &log_dir, const std::string &log_level, const std::string &log_name) : 
    log_dir(log_dir), log_level(log_level), log_name(log_name), log_out_file(log_dir+"/"+log_name+".log"), log_err_file(log_dir+"/"+log_name+".log.err")
{
}

xrtc::XrtcLog::~XrtcLog() {
    stop_log_thread();
    if(_out_file.is_open()){
        _out_file.close();
    }
    if(_err_file.is_open()){
        _err_file.close();
    }
}

static rtc::LoggingSeverity GetLogSeverity(const std::string &log_level)
{
    std::cout<<"log_level:"<<log_level<<std::endl;
    if (log_level == "INFO")
    {
        return rtc::LS_INFO;
    }
    else if (log_level == "WARNING")
    {
        return rtc::LS_WARNING;
    }
    else if (log_level == "ERROR")
    {
        return rtc::LS_ERROR;
    }
    else if (log_level == "VERBOSE")
    {
        return rtc::LS_VERBOSE;
    }
    else if (log_level == "NONE")
    {
        return rtc::LS_NONE;
    }
    else
    {
        return rtc::LS_INFO;
    }
    return rtc::LS_NONE;
}

int xrtc::XrtcLog::InitLog()
{
    rtc::LogMessage::ConfigureLogging("thread tstamp");
    rtc::LogMessage::SetLogPathPrefix("/src");
    rtc::LogMessage::AddLogToStream(this, GetLogSeverity(log_level));

    int ret = mkdir(log_dir.c_str(), 0755);
    if(ret != 0 && errno != EEXIST) {
        std::cerr<<"mkdir log dir failed "<<log_dir<<std::endl;
        return -1;
    }

    _out_file.open(log_out_file, std::ios::app);
    if(!_out_file.is_open()) {
        std::cerr<<"open out log file failed"<<std::endl;
        return -1;
    }

    _err_file.open(log_err_file, std::ios::app);
    if(!_err_file.is_open()) {
        std::cerr<<"open err log file failed"<<std::endl;
        return -1;
    }
    return 0;
}

bool xrtc::XrtcLog::start_log_thread(){
    if(_log_thread_running){
        std::cerr<<"log thread is already running"<<std::endl;
        return false;
    }

    _log_thread_running = true;
    _log_thread = new std::thread([=](){
        while(_log_thread_running){
            struct stat st;
            if(stat("log_dir.c_str()", &st)<0){
                _out_file.close();
                _out_file.open(log_out_file, std::ios::app);
            }

            if(stat("log_err_file.c_str()", &st)<0){
                _err_file.close();
                _err_file.open(log_err_file, std::ios::app);
            }
            {
                std::unique_lock<std::mutex> lock(_log_mutex);
                while(!_log_queue.empty()){
                    _out_file<<_log_queue.front()<<std::endl;
                    _log_queue.pop();
                }
                _out_file.flush();
            }
            {
                std::unique_lock<std::mutex> lock(_err_mutex);
                while(!_err_queue.empty()){
                    _err_file<<_err_queue.front()<<std::endl;
                    _err_queue.pop();
                }
                _err_file.flush();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            

        }
    });

    return true;

}

void xrtc::XrtcLog::stop_log_thread(){
    _log_thread_running = false;

    if(_log_thread){
        if(_log_thread->joinable()){
            _log_thread->join();
        }
        delete _log_thread;
        _log_thread = nullptr;
    }
}

void xrtc::XrtcLog::join_log_thread(){
    if(_log_thread ){
        if(_log_thread->joinable()){
            _log_thread->join();
        }
    }
}


void XrtcLog::set_log_to_stderr(bool log_to_stderr)
{
    rtc::LogMessage::SetLogToStderr(log_to_stderr);
}

void xrtc::XrtcLog::OnLogMessage(const std::string &message,rtc::LoggingSeverity severity) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = *std::localtime(&now_time);

    std::cout << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << " - " << message << std::endl;
    std::stringstream ss ;
    ss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    const std::string log_message = ss.str()+" - " +message;

    if(severity>=rtc::WARNING){
        std::unique_lock<std::mutex> lock(_err_mutex);
        _err_queue.push(log_message);
    }else{
        std::unique_lock<std::mutex> lock(_log_mutex);
        _log_queue.push(log_message);
    }
}

void xrtc::XrtcLog::OnLogMessage(const std::string& message){

}
}