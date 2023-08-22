//
// Created by micro on 2023/8/12.
//

#include "FormatCtx.h"
#include <iostream>
using namespace std;
FormatCtx::~FormatCtx() {
    if (fmt) {
        if(fmt->pb) {
            avio_flush(fmt->pb);
            avio_close(fmt->pb);
        }
        avformat_free_context(fmt);
    }
    for (CodecCtx *item: codecs) {
        delete item;
    }
    codecs.clear();
}

void logFormat(const AVCodec *vCodec) {
    if (vCodec->pix_fmts == nullptr) {
        return;
    }
    const enum AVPixelFormat *pix_fmts = vCodec->pix_fmts;
    while (*pix_fmts  != AV_PIX_FMT_NONE) {
        const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(*pix_fmts);
        cout << desc->name << endl;
        pix_fmts++;
    }
};

FormatCtx::FormatCtx() : fmt(nullptr)
                         {
}

void FormatCtx::dumpFmt() {
    for (int i = 0; i < fmt->nb_streams; ++i) {
        av_dump_format(fmt, i, fmt->url, fmt->iformat == nullptr);
    }
}

int FormatCtx::open() const {
    if (fmt->pb != nullptr) {
        return -1;
    }
    int ret;
    ret = avio_open(&fmt->pb,fmt->url,fmt->oformat == nullptr ? AVIO_FLAG_READ : AVIO_FLAG_WRITE);
    if (ret < 0) {
        return ret;
    }
    if (fmt->oformat != nullptr) {
        ret = avformat_write_header(fmt, nullptr);
    }
    return ret;
}

int FormatCtx::initBy(const char *filename) {
    return avformat_open_input(&fmt, filename, nullptr,nullptr);
}

int FormatCtx::close() {
    avformat_close_input(&fmt);
    return 0;
}

int FormatOutput::onPackage(AVPacket *pkg) {
    int ret = av_interleaved_write_frame(fmt,pkg);
    if (ret < 0) {
        CodecCtx::printErr(ret);
        return ret;
    }
    return ret;
}


FormatOutput::FormatOutput()= default;

int FormatOutput::initBy(const char *filename) {
    return avformat_alloc_output_context2(&fmt, nullptr,nullptr,filename);
}

int FormatOutput::addStream(CodecCtx *ctx) {
    int ret = -1;
    if (ctx == nullptr) {
        return ret;
    }

    if (fmt->oformat->flags & AVFMT_GLOBALHEADER)
        ctx->ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = ctx->open();
    if (ret < 0) {
        return ret;
    }

    ctx->stream = avformat_new_stream(fmt, ctx->ctx->codec);
    if (!ctx->stream) {
        return ret;
    }
    ret = avcodec_parameters_from_context( ctx->stream->codecpar, ctx->ctx);
    if (ret < 0) {
        return ret;
    }
    ctx->stream->avg_frame_rate = ctx->ctx->framerate;
    ctx->stream->time_base = ctx->ctx->time_base;
    return ret;
}

VideoOutCodecCtx *FormatOutput::newVideo(enum AVCodecID codecId, int rate) {
    VideoOutCodecCtx *vCodec = VideoOutCodecCtx::findById(codecId);
    if (vCodec == nullptr) {
        return nullptr;
    }
    AVCodecContext *vCtx = vCodec->ctx;
    vCtx->time_base = av_make_q(1, rate);
    //设置GOP大小，即连续B帧最大数目
    vCtx->gop_size= 300;
    vCtx->bit_rate = 800000;
    vCtx->max_b_frames = 16;
//    vCtx->qcompress = 1.0;
    vCtx->framerate = av_inv_q(vCtx->time_base);

    if(vCtx->codec->id == AV_CODEC_ID_H264) {
        av_opt_set(vCtx->priv_data, "preset", "slow", 0);
    }
    //设置量化系数
    vCtx->qmin=10;
    vCtx->qmax=51;

    codecs.push_back(vCodec);
    return vCodec;
}

FormatOutput::~FormatOutput() = default;


SubTitle * FormatOutput::newSubTitle(AVCodecID id, size_t buffSize) {
    SubTitle *codec = SubTitle::findById(id, true,buffSize);
    if (codec) {
        codecs.push_back(codec);
    }
    return codec;
}

int FormatOutput::close() {
    int ret = 0;
    if (fmt->pb) {
        ret = av_write_trailer(fmt);
        avio_flush(fmt->pb);
        avio_close(fmt->pb);
        fmt->pb = nullptr;
    }
    return ret;
}

