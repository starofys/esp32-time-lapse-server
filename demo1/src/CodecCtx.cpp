#include "CodecCtx.h"

CodecCtx::CodecCtx(enum AVCodecID id,bool encoder) {
    if (encoder) {
        codec = avcodec_find_encoder(id);
    } else {
        codec = avcodec_find_decoder(id);
    }
    if (codec) {
        ctx = avcodec_alloc_context3(codec);
    }
}

CodecCtx::~CodecCtx() {
    if (ctx) {
        avcodec_close(ctx);
        avcodec_free_context(&ctx);
    }
}

int CodecCtx::open() const {
    return avcodec_open2(ctx,ctx->codec, nullptr);
}

int CodecCtx::initVideo(int width,int height,int rate,enum AVPixelFormat pix_fmt) {
    int ret = 0;
    ctx->width = width;
    ctx->height = height;
    ctx->time_base = av_make_q(1,rate);
    ctx->pix_fmt = pix_fmt;
    //设置GOP大小，即连续B帧最大数目
    ctx->gop_size=5;
    ctx->bit_rate = 800000;
    ctx->max_b_frames = 1;
    ctx->qcompress = 1.0;
//    ctx->max_b_frames = 1000;
    ctx->framerate = av_inv_q(ctx->time_base);

    if(ctx->codec->id == AV_CODEC_ID_H264) {
        av_opt_set(ctx->priv_data, "preset", "slow", 0);
    }
    //设置量化系数
//    ctx->qmin=10;
//    ctx->qmax=51;

    if (ctx->flags & AVFMT_GLOBALHEADER)
        ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    return ret;
}
