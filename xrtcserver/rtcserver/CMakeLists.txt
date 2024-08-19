cmake_minimum_required(VERSION 3.5)
project(rtcserver)

add_definitions("-g -pipe -W -Wall -fPIC -std=gnu++17 -DWEBRTC_POSIX
    -DWEBRTC_LINUX")

file(GLOB all_src
    "./src/*.cpp"
)

add_executable(rtcserver ${all_src})

