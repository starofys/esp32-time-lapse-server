#include "demo1.h"
#include <iostream>
extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
}
void version()
{
    std::cout << av_version_info() << std::endl;
}