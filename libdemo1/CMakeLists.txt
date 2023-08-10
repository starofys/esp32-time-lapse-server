cmake_minimum_required(VERSION 3.25)
project(libdemo1)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")


aux_source_directory(./src SRC)

include_directories(./include)

add_library(libdemo1 SHARED ${SRC} )
target_link_libraries(libdemo1 ${FFmpeg_LIBRARIES})
