//
// Created by micro on 2023/9/2.
//

#ifndef CPP_PROJECTS_CAPTUREAPP_H
#define CPP_PROJECTS_CAPTUREAPP_H
#include "FormatCtx.h"
#include "UdpCodec.h"

class CaptureApp : public FrameSink ,public PacketSink,public JpgListener {
private:
    char filename[50];
    FormatOutput fOut;
    InCodecCtx* inCtx;
    VideoOutCodecCtx *outCtx;
    SubTitle* subTitle = nullptr;
    SubTitle* webVtt = nullptr;
    FormatOutput *webVttOut;
    std::string subTitleName;
    int lastRet = 0;
public:
    CaptureApp(): webVttOut(nullptr){};
    int initParams(std::string &encoder,std::string& extFile, int rate,bool webvtt);
    ~CaptureApp() override;
    int release();
    int onImage(const char *buff, int len) override;
    int valid() override;
    int onFrame(CodecCtx *codecCtx,AVFrame* frame) override;
    int onPackage(AVPacket* _pkg) override {
        return 0;
    }
};


#endif //CPP_PROJECTS_CAPTUREAPP_H
