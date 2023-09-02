//
// Created by micro on 2023/9/2.
//

#include "CaptureApp.h"
#include <iostream>
using namespace std;

int CaptureApp::initParams(std::string &encoder,std::string &extFile, int rate,bool webvtt) {
    memset(filename,0,sizeof(filename));
    time_t currentTim = time(nullptr);
    strftime(filename, sizeof(filename), "%Y%m%d_%H_%M_%S",localtime(&currentTim));
    sprintf(filename,"%s.%s",filename,extFile.c_str());

    cout << "encoder = " << encoder << " outfile = " << filename << endl;
    int ret = fOut.initBy(filename);
    if (ret < 0) {
        return ret;
    }
    // 输入解码器
    inCtx = InCodecCtx::findById(AV_CODEC_ID_MJPEG);
    inCtx->ctx->time_base = av_make_q(1,rate);
    // 输入对象
    inCtx->setFrameSink(this);

    // 输出编码器
    outCtx = VideoOutCodecCtx::findByName(encoder.c_str());
    outCtx->initDefault(rate);
    // 输出对象
    outCtx->setPacketSink(&fOut);

    // 字幕编码器 用于记录时间
    enum AVCodecID subTitleCodec = AV_CODEC_ID_MOV_TEXT;
    if (fOut.fmt->oformat->video_codec == AV_CODEC_ID_FLV1) {
        subTitleCodec = AV_CODEC_ID_TEXT;
    }
    subTitle = SubTitle::findById(subTitleCodec, true,200);
    subTitle->ctx->time_base = outCtx->ctx->time_base;
    subTitle->ctx->framerate  = outCtx->ctx->framerate;

    if (webvtt) {
        webVtt = SubTitle::findById(AV_CODEC_ID_WEBVTT, true,200);
        webVtt->ctx->time_base = outCtx->ctx->time_base;
        webVtt->ctx->framerate  = outCtx->ctx->framerate;

        webVttOut = new FormatOutput;

        subTitleName = filename;
        subTitleName.replace(subTitleName.find(extFile), extFile.length(),"vtt");
        ret = webVttOut->initBy(subTitleName.c_str());

        if (ret >=0) {
            ret = webVttOut->addStream(webVtt);
        }
        if (ret>=0) {
            webVtt->setPacketSink(webVttOut);
        } else {
            delete webVttOut;
            webVttOut = nullptr;
        }
    }

    return inCtx->open();
}

int CaptureApp::onImage(const char *buff, int len) {
    int ret;
    AVPacket *pkg = av_packet_alloc();
    pkg->data = (uint8_t*)buff;
    pkg->size = len;
    //pkg.time_base = sourceRate;
    pkg->pts = fOut.pts;
    pkg->dts = fOut.pts;
    fOut.pts++;

    ret = inCtx->onPackage(pkg);
    pkg->data = nullptr;
    pkg->size = 0;

    av_packet_free(&pkg);
    return ret;
}

int CaptureApp::onFrame(CodecCtx *codecCtx, AVFrame *frame) {
    int ret = 0;
    if (!fOut.fmt->pb) {
        ret = outCtx->initVideo(frame);
        if (ret < 0) {
            return ret;
        }
        ret = fOut.addStream(outCtx);
        if (ret < 0) {
            exit(ret);
            return ret;
        }
        if (subTitle) {
            ret = fOut.addStream(subTitle);
            if (ret <0 ) {
                return ret;
            }
            subTitle->setPacketSink(&fOut);
        }
        ret = fOut.open();
        if (ret < 0) {
            CodecCtx::printErr(ret);
            exit(ret);
        }

        if (webVttOut) {
            if (webVttOut->open() < 0) {
                delete webVttOut;
                webVttOut = nullptr;
            }
        }

    }

    ret = outCtx->onFrame(codecCtx,frame);

    int duration = inCtx->ctx->time_base.den * 5;
    if (subTitle && frame->pts % duration == 0) {
        frame->duration = duration;
        time_t currentTim = time(nullptr);
        strftime(filename, sizeof(filename), "%Y-%m-%d %H:%M:%S",localtime(&currentTim));
        subTitle->encodeTxt(frame,filename);
        if (webVttOut) {
            webVtt->encodeTxt(frame,filename);
        }
    }



    return ret;
}

CaptureApp::~CaptureApp() {
    this->release();
}

int CaptureApp::release() {
    int ret;
    if (fOut.fmt->pb) {
        ret = outCtx->onFrame(inCtx,nullptr);
        if (ret <0) {
            CodecCtx::printErr(ret);
        }
    }
    ret = fOut.close();
    if (ret <0) {
        CodecCtx::printErr(ret);
    }
    if (subTitle) {
        delete subTitle;
        subTitle = nullptr;
    }
    if (inCtx) {
        delete inCtx;
        inCtx = nullptr;
    }
    if (outCtx) {
        delete outCtx;
        outCtx = nullptr;
    }
    if (webVttOut) {
        webVttOut->close();
        delete webVttOut;
    }
    if (webVtt) {
        delete webVtt;
        webVtt = nullptr;
    }
    return ret;
}
