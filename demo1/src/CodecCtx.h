//
// Created by micro on 2023/8/12.
//

#ifndef CPP_PROJECTS_CODECCTX_H
#define CPP_PROJECTS_CODECCTX_H
#include "demo1.h"

class CodecCtx {
private:
public:
    AVCodecContext *ctx;
    const AVCodec *codec;
    AVStream *stream;
    explicit CodecCtx(enum AVCodecID id,bool encoder);
    ~CodecCtx();
    int open() const;
    int initVideo(int width,int height,int rate,enum AVPixelFormat pix_fmt);
};


#endif //CPP_PROJECTS_CODECCTX_H
