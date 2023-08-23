cmake_minimum_required(VERSION 3.16)
#set (CMAKE_C_COMPILER "gcc.exe")
#set (CMAKE_CXX_COMPILER "g++.exe")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")

project(cpp_projects C CXX)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FFmpeg REQUIRED IMPORTED_TARGET
        libavdevice
        libavformat
        libavfilter
        libavcodec
        libswresample
        libswscale
        libavutil)
if(FFmpeg_FOUND)
    message(${FFmpeg_LIBRARIES})
    message(${FFmpeg_INCLUDE_DIRS})
if (WIN32)
    message(${FFmpeg_LIBRARY_DIRS})
endif()
    include_directories(${FFmpeg_INCLUDE_DIRS})
endif()

include_directories(include)
add_subdirectory(go_ffmpeg)
add_subdirectory(demo1)

# add_subdirectory(third_partys/asio2)