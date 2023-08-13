#include "demo1.h"
#include <iostream>
extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
}
void c_do_log()
{
    const char* str = av_version_info();
    std::cout << "Hello, World! " << str << std::endl;

    AVFormatContext *pFormatContext = avformat_alloc_context();

    std::cout << "Hello, World! " << pFormatContext << std::endl;

}