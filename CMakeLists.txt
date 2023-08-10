cmake_minimum_required(VERSION 3.25)
project(cpp_projects)
set(CMAKE_CXX_STANDARD 14)

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
    message(${FFmpeg_LIBRARY_DIRS})
endif()


add_subdirectory(libdemo1)
add_subdirectory(demo1)