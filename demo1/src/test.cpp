#include "demo1.h"
#include <iostream>
#include <fstream>
#include "CodecCtx.h"
#include "FormatCtx.h"
using namespace std;
AVPacket* readAll(const char* filename) {
    cout << filename <<endl;
    ifstream file;
    file.open(filename,ios::in|ios::binary);
    if (!file.is_open()) {
        return nullptr;
    }
    file.seekg(0, std::ios::end);
    int fileLength = file.tellg();
    //指定定位到文件开始
    file.seekg(0, std::ios::beg);
    cout << "fileLength:" << fileLength << endl;
    auto * buffer = new uint8_t[fileLength + 1];
    file.read((char*)buffer, fileLength);
    AVPacket *pkg = av_packet_alloc();
    av_packet_from_data(pkg,buffer,fileLength);
    return pkg;
}

int main2() {
    const char *outfile = "output.mp4";
    int rate = 25;
    int ret;
    AVRational sourceRate = av_make_q(1, rate);
    // 读取图片
    int num = 2;
    AVPacket *pkgList[num];
    for (int i = 0; i < num; i++) {
        string name = "H:/";
        name += std::to_string(i);
        name += ".jpg";
        pkgList[i] = readAll(name.c_str());
    }
    return 0;
}

int main3() {
    auto webVtt = SubTitle::findById(AV_CODEC_ID_WEBVTT, true,200);

    webVtt->ctx->time_base = av_make_q(1,25);
    webVtt->ctx->framerate  = av_make_q(25,1);

    auto webVttOut = new FormatOutput;
    int ret = webVttOut->initBy("test.vtt");
    if (ret >=0) {
        ret = webVttOut->addStream(webVtt);
    }
    if (ret>=0) {
        webVtt->setPacketSink(webVttOut);
    }
    if (ret>=0) {
        ret = webVttOut->open();
    }
    if (ret>=0) {
        AVFrame frame;
        string str;

        for (int i = 0; i < 10; ++i) {
            frame.pts = i *  webVtt->ctx->time_base.den;
            frame.pkt_dts = frame.pts;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 2, 100)
            frame.duration = webVtt->ctx->time_base.den;
#else
            frame.pkt_duration = webVtt->ctx->time_base.den;
#endif
            str = to_string(i);
            webVtt->encodeTxt(&frame,str.c_str());
        }

        webVttOut->close();
    }
    delete webVttOut;
    delete webVtt;

    return ret;
}