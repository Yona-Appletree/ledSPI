cmake_minimum_required(VERSION 3.2)
project(SPIscape)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    lib/cesanta/frozen.c
    lib/cesanta/frozen.h
    lib/cesanta/mongoose.c
    lib/cesanta/mongoose.h
    lib/cesanta/net_skeleton.h
    ledspi-server.c
    spio.h
    spio.c
    util.c
    util.h)

add_executable(SPIscape ${SOURCE_FILES})