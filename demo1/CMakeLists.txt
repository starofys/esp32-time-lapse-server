cmake_minimum_required(VERSION 3.25)
project(demo1)
set(CMAKE_CXX_STANDARD 11)


aux_source_directory(./src SRC)
include_directories(./include)


add_executable(demo1 ${SRC})
target_link_libraries(demo1 ${FFmpeg_LIBRARIES})
