cmake_minimum_required(VERSION 3.5)
project(rtcserver)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")


add_definitions("-g -pipe -W -Wall -fPIC -std=gnu++20 -DWEBRTC_POSIX
    -DWEBRTC_LINUX")

include_directories(
    "./src"
    "./thirdparty/include"
    "../rtcbase/src"
)

link_directories(
    "./thirdparty/lib"
    "../rtcbase/out"
)

file(GLOB all_src
    "./src/*.cpp"
    "./src/base/*.cpp"
    "./src/signaling/*.cpp"
)

add_executable(rtcserver ${all_src})

target_link_libraries(rtcserver libyaml-cpp.a librtcbase.a -lpthread)
target_link_libraries(rtcserver libabsl_strings.a libabsl_throw_delegate.a)
target_link_libraries(rtcserver libev.a)

