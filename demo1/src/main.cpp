#include <iostream>
#include <fstream>
#include "demo1.h"
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
class FrameInit : public FrameSink {
private:
    FormatOutput *out;
    VideoOutCodecCtx *outCtx;
public:
    SubTitle* subTitle;
    InCodecCtx *inCtx = nullptr;
    FrameInit(FormatOutput *_out,VideoOutCodecCtx *_codecCtx): out(_out),outCtx(_codecCtx) {};
    int onFrame(CodecCtx *codecCtx,AVFrame* frame) override {
        int ret = 0;
        if (!out->fmt->pb) {
            ret = outCtx->initVideo(frame);
            if (ret < 0) {
                return ret;
            }
            ret = out->addStream(outCtx);
            if (ret >=0) {
                ret = out->addStream(subTitle);
                if (ret <0 ) {
                    return ret;
                }
            }
            if (ret >= 0) {
                ret = out->open();
            }
            if (ret >=0) {
                if (inCtx) {
                    inCtx->setFrameSink(outCtx);
                }
            }
        }
        if (ret >=0) {
            ret = outCtx->onFrame(codecCtx,frame);
        }
        return ret;
    }
};
class TimeLapseVideo {

};
int main() {
    const char* outfile = "output.mp4";
    int rate = 25;
    int ret;
    AVRational sourceRate =  av_make_q(1,rate);
    // 读取图片
    int num = 2;
    AVPacket* pkgList[num];
    for (int i =0;i<num;i++) {
        string name = "H:/";
        name += std::to_string(i);
        name+=".jpg";
        pkgList[i] = readAll(name.c_str());
    }

    InCodecCtx* inCtx = InCodecCtx::findById(AV_CODEC_ID_MJPEG);
    inCtx->ctx->time_base = sourceRate;
    ret = inCtx->open();
    if (ret < 0) {
        return ret;
    }


    FormatOutput outFmt;
    ret = outFmt.initBy(outfile);
    if (ret < 0) {
        return ret;
    }
    VideoOutCodecCtx *outCtx = outFmt.newVideo(AV_CODEC_ID_HEVC,rate);
    FrameInit frameInit(&outFmt,outCtx);
    inCtx->setFrameSink(&frameInit);
    frameInit.inCtx =  inCtx;
    outCtx->setPacketSink(&outFmt);

    SubTitle *subTitle = outFmt.newSubTitle(AV_CODEC_ID_MOV_TEXT, 512);
    subTitle->ctx->time_base = outCtx->ctx->time_base;
    subTitle->ctx->framerate  = outCtx->ctx->framerate;

    frameInit.subTitle = subTitle;


    int index = 0;

    for (int i = 0; i < rate * 10; ++i) {
        AVPacket* pkg = pkgList[index];
        pkg->time_base = sourceRate;
        pkg->pts = outFmt.pts;
        pkg->dts = outFmt.pts;
        outFmt.pts++;
        if (i % rate ==0) {
            index++;
            if (index == num) {
                index = 0;
            }
        }
//        if (i % rate !=0) {
//            continue;
//        }

        ret = inCtx->onPackage(pkg);
        if (ret < 0) {
            return ret;
        }

    }
    ret = outCtx->onFrame(inCtx,nullptr);
    if (ret <0) {
        CodecCtx::printErr(ret);
    }
    ret = outFmt.close();
    return ret;

}

