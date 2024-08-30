#pragma once

#include <cstdint>
namespace xrtc
{ 
    const int XHEAD_SIZE = 36;
    const uint32_t XHEAD_MAGIC = 0xfb202402;

    struct xhead_t
    {
        uint16_t id;
        uint16_t version;
        uint32_t log_id;
        char provider[16];
        uint32_t magic_num;
        uint32_t reserved;
        uint32_t body_len;
    };

}