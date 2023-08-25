#include <iostream>
#include <fstream>
#include "demo1.h"
#include "CodecCtx.h"
#include "FormatCtx.h"
#include "UdpServer.h"
#include <csignal>
#include "UdpCodec.h"

using namespace std;
char filename[256];
class FrameInit : public FrameSink ,public PacketSink,public JpgListener {
private:
    FormatOutput *out;
    VideoOutCodecCtx *outCtx;
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
        auto* data = (uint8_t*)av_malloc(len);
        memcpy(data,buff,len);
        ret = av_packet_from_data(&pkg,data,len);
        if (ret < 0) {
            cout << "pkg err" <<endl;
            av_free(data);
            return;
        }

        //pkg.time_base = sourceRate;
        pkg.pts = out->pts;
        pkg.dts = out->pts;
        out->pts++;

        ret = inCtx->onPackage(&pkg);
        if (ret < 0) {
            cout << "encode pkg err" <<endl;
        }
        av_packet_unref(&pkg);
    }
    int onFrame(CodecCtx *codecCtx,AVFrame* frame) override {
        int ret = 0;
        if (!out->fmt->pb) {
            ret = outCtx->initVideo(frame);
            if (ret < 0) {
                return ret;
            }
            ret = out->addStream(outCtx);
            if (ret < 0) {
                exit(ret);
                return ret;
            }
            if (subTitle) {
                ret = out->addStream(subTitle);
                if (ret <0 ) {
                    return ret;
                }
                subTitle->setPacketSink(out);
            }
            ret = out->open();
            if (ret < 0) {
                CodecCtx::printErr(ret);
                exit(ret);
                return ret;
            }
        }
        int duration = sourceRate.den;
        if (subTitle && frame->pts % duration == 0) {
            time_t currentTim = time(nullptr);
            strftime(filename, sizeof(filename), "%Y-%m-%d %H:%M:%S",localtime(&currentTim));
            subTitle->encodeTxt(frame,filename);
        }

        ret = outCtx->onFrame(codecCtx,frame);

        return ret;
    }
    int onPackage(AVPacket* _pkg) override {
        return 0;
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
int capture(const char *encoder,const char* extFile, int rate) {
    int ret = -1;
    memset(filename,0,sizeof(filename));
    time_t currentTim = time(nullptr);
    strftime(filename, sizeof(filename), "%Y%m%d_%H_%M_%S",localtime(&currentTim));
    sprintf(filename,"%s.%s",filename,extFile);
    const char* outfile = filename;
    cout << "encoder = " << encoder << " outfile = " << outfile << endl;
    AVRational sourceRate =  av_make_q(1,rate);
    InCodecCtx* inCtx = InCodecCtx::findById(AV_CODEC_ID_MJPEG);
    inCtx->ctx->time_base = sourceRate;
    ret = inCtx->open();
    if (ret < 0) {
        delete inCtx;
        return ret;
    }

    FormatOutput outFmt;
    ret = outFmt.initBy(outfile);
    if (ret < 0) {
        delete inCtx;
        return ret;
    }
    VideoOutCodecCtx *outCtx = VideoOutCodecCtx::findByName(encoder);
    outCtx->initDefault(sourceRate.den);

    auto frameInit = new FrameInit(&outFmt,outCtx,sourceRate);
    inCtx->setFrameSink(frameInit);
    frameInit->inCtx =  inCtx;
    outCtx->setPacketSink(&outFmt);
    enum AVCodecID subTitleCodec = AV_CODEC_ID_MOV_TEXT;
    if (outFmt.fmt->oformat->video_codec == AV_CODEC_ID_FLV1) {
        subTitleCodec = AV_CODEC_ID_TEXT;
    }
    SubTitle *subTitle = SubTitle::findById(subTitleCodec, true,512);
    subTitle->ctx->time_base = outCtx->ctx->time_base;
    subTitle->ctx->framerate  = outCtx->ctx->framerate;
    frameInit->subTitle = subTitle;

    app.setJpgListener(frameInit);
    server.setListener(&app);

    server.loop();
    server.setListener(nullptr);
    app.setJpgListener(nullptr);
    frameInit->release();
    if (frameInit->subTitle) {
        delete frameInit->subTitle;
    }
    delete frameInit;
    delete inCtx;
    delete outCtx;
    ret = 0;
    return ret;
}
int main (int argc,char*argv[])
{
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    int rate = 25;
    const char* encoder = "libx264";
    const char* extFile = "mp4";
    if (argc > 1) {
        encoder = argv[1];
    }
    if (argc > 2) {
        rate = std::atoi(argv[2]);
    }
    if (argc > 3) {
        extFile = argv[3];
    }
    int ret;
    ret = server.open(8080);
    if (ret !=0) {
        return ret;
    }

    ret = capture(encoder, extFile,rate);


    return ret;

}

