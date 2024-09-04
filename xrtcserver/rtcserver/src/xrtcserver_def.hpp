#pragma once
#include <string>
#include <cstdint>
#define CMDNO_PUSH 1
#define CMDNO_PULL 2
#define CMDNO_ANSWER 3
#define CMDNO_STOPPUSH 4
#define CMDNO_STOPPULL 5

namespace xrtc {
    struct RtcMsg{
        int cmdno=-1;
        uint64_t uid=0;
        std::string stream_name;
        int audio=0;
        int video=0;
        u_int32_t log_id=0;

        void* worker=nullptr;
        void* conn=nullptr;
        std::string sdp;
        int err_no=0;
    };
}