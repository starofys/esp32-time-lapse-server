//
// Created by micro on 2023/8/12.
//

#include "FormatCtx.h"

FormatCtx::~FormatCtx() {
    if (fmt) {
        if(fmt->pb) {
            avio_flush(fmt->pb);
            avio_close(fmt->pb);
        }
        avformat_free_context(fmt);
    }

}

int FormatCtx::newOutput(const char *name){
    is_output = 1;
    this->filename = name;
    return avformat_alloc_output_context2(&fmt, nullptr, nullptr,this->filename);
}

//int FormatCtx::newInput(const char *name){
//    is_output = 0;
//    this->filename = name;
//    return avformat_alloc_input_context2(&fmt, nullptr, nullptr,this->filename);
//}

FormatCtx::FormatCtx() {

}

int FormatCtx::newStream(CodecCtx *ctx) {
    ctx->stream = avformat_new_stream(fmt, ctx->codec);
    int ret = -1;
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

void FormatCtx::dumpFmt() {
    av_dump_format(fmt, 0, filename, is_output);
}

int FormatCtx::open() {
    int ret;
    ret = avio_open(&fmt->pb,filename,is_output == 1 ? AVIO_FLAG_WRITE:AVIO_FLAG_READ);
    if (ret < 0) {
        return ret;
    }
    ret = avformat_write_header(fmt, nullptr);
    return ret;
}

