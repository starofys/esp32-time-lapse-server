cmake_minimum_required(VERSION 3.16)
project(go_ffmpeg)
set(CMAKE_CXX_STANDARD 17)

message(${FFmpeg_LIBRARY_DIRS})

include_directories(../include)
aux_source_directory(./src SRC)

add_library(go_ffmpeg SHARED ${SRC} )
target_link_libraries(go_ffmpeg PUBLIC ${FFmpeg_LIBRARIES})
target_link_directories(go_ffmpeg PUBLIC ${FFmpeg_LIBRARY_DIRS})

set(LIBRARY_OUTPUT_PATH ../bin)