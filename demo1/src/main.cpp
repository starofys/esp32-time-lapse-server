#include <iostream>
#include <fstream>
#include "demo1.h"
#include "CodecCtx.h"
#include "FormatCtx.h"
#include "UdpServer.h"
#include <csignal>
#include "UdpCodec.h"

using namespace std;

class FrameInit : public FrameSink ,public JpgListener {
private:
    FormatOutput *out;
    VideoOutCodecCtx *outCtx;
    int index = 0;
    AVPacket pkg = {nullptr};
    AVRational sourceRate;
public:
    SubTitle* subTitle = nullptr;
    InCodecCtx *inCtx = nullptr;
    FrameInit(FormatOutput *_out,VideoOutCodecCtx *_codecCtx,AVRational _sourceRate):
        out(_out),outCtx(_codecCtx),sourceRate(_sourceRate) {
    };
    ~FrameInit() {
    }
    void release() {
        int ret;
        if (out->fmt->pb) {
            ret = outCtx->onFrame(inCtx,nullptr);
            if (ret <0) {
                CodecCtx::printErr(ret);
            }
        }
        ret = out->close();
        if (ret <0) {
            CodecCtx::printErr(ret);
        }
    }
    void onImage(const char *buff, int len) override {
        int ret;
        ret = av_packet_from_data(&pkg,(uint8_t*)buff,len);
        if (ret < 0) {
            cout << "pkg err" <<endl;
        }

        //pkg.time_base = sourceRate;
        pkg.pts = out->pts;
        pkg.dts = out->pts;
        out->pts++;

        ret = inCtx->onPackage(&pkg);
        if (ret < 0) {
            cout << "onPackage err" <<endl;
        }
    }
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
UdpServer server(1500);
UdpCodec app(200000);
void SignalHandler(int signal)
{
    printf("shutdown %d\n",signal);
    if (signal == SIGINT || signal ==SIGTERM ) {
        server.release();
        app.release();
    }
}

int main ()
{
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    int ret;
    ret = server.open(8080);
    if (ret !=0) {
        return ret;
    }

    char filename[256];
    memset(filename,0,sizeof(filename));
    time_t currentTim = time(nullptr);
    strftime(filename, sizeof(filename), "%Y%m%d_%H_%M_%S.mp4",localtime(&currentTim));
    const char* outfile = filename;
    int rate = 25;
    cout << "outfile = " << outfile << endl;
    AVRational sourceRate =  av_make_q(1,rate);
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
    VideoOutCodecCtx *outCtx = outFmt.newVideo(AV_CODEC_ID_H264,rate);
    auto *frameInit = new FrameInit(&outFmt,outCtx,sourceRate);
    inCtx->setFrameSink(frameInit);
    frameInit->inCtx =  inCtx;
    outCtx->setPacketSink(&outFmt);

    SubTitle *subTitle = outFmt.newSubTitle(AV_CODEC_ID_MOV_TEXT, 512);
    subTitle->ctx->time_base = outCtx->ctx->time_base;
    subTitle->ctx->framerate  = outCtx->ctx->framerate;

    frameInit->subTitle = subTitle;
    app.setJpgListener(frameInit);
    server.setListener(&app);

    server.loop();

    frameInit->release();
    delete frameInit;

    return ret;

}

