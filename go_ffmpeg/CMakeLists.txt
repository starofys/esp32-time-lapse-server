cmake_minimum_required(VERSION 3.16)
project(go_ffmpeg)
set(CMAKE_CXX_STANDARD 17)


include_directories(../include)
aux_source_directory(./src SRC)

add_library(go_ffmpeg SHARED ${SRC} )
target_link_libraries(go_ffmpeg ${FFmpeg_LIBRARIES})
set(LIBRARY_OUTPUT_PATH ../bin)