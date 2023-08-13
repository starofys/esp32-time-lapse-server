cmake_minimum_required(VERSION 3.16)
project(demo1)
set(CMAKE_CXX_STANDARD 11)

aux_source_directory(./src SRC)
include_directories(./include)
include_directories(${FFmpeg_INCLUDE_DIRS})

add_executable(demo1 ${SRC})
target_link_libraries(demo1 go_ffmpeg ${FFmpeg_LIBRARIES})
set(EXECUTABLE_OUTPUT_PATH  ../bin)