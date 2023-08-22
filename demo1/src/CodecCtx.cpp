#include "CodecCtx.h"
#include <iostream>
using namespace std;
CodecCtx::CodecCtx(AVCodecContext *_ctx) :ctx(_ctx) {
    stream = nullptr;
}

CodecCtx::~CodecCtx() {
    if (ctx) {
        avcodec_close(ctx);
        avcodec_free_context(&ctx);
    }

}

void CodecCtx::printErr(int ret) {
    char a[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    av_make_error_string(a, AV_ERROR_MAX_STRING_SIZE, ret);
    cout << a << endl;
}

int CodecCtx::open() const {
    int ret = avcodec_open2(ctx,ctx->codec, nullptr);
    if (ret < 0) {
        CodecCtx::printErr(ret);
    }
    return ret;
}

AVCodecContext* CodecCtx::findById(enum AVCodecID id, bool encoder) {
    const AVCodec *codec;
    if (encoder) {
        codec = avcodec_find_encoder(id);
    } else {
        codec = avcodec_find_decoder(id);
    }
    if (codec) {
        return avcodec_alloc_context3(codec);
    }
    return nullptr;

}
InCodecCtx* InCodecCtx::findById(enum AVCodecID id) {
    auto ctx = CodecCtx::findById(id, false);
    if (ctx != nullptr) {
        return new InCodecCtx(ctx);
    }
    return nullptr;
}
int InCodecCtx::onPackage(AVPacket *inPkt) {
    int ret = avcodec_send_packet(ctx,inPkt);
    if (ret < 0) {
        CodecCtx::printErr(ret);
        return ret;
    }
    do {
        ret = avcodec_receive_frame(ctx, frame);
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
            ret = 0;
            break;
        }
        frame->key_frame = 0;
        frame->pict_type = AV_PICTURE_TYPE_NONE;
        if (sink) {
            ret = sink->onFrame(this,frame);
        }
        av_frame_unref(frame);
    } while (ret >= 0);
    if (ret < 0) {
        CodecCtx::printErr(ret);
    }
    return ret;
}
OutCodecCtx* OutCodecCtx::findById(enum AVCodecID id) {
    auto ctx = CodecCtx::findById(id, true);
    if (ctx != nullptr) {
        return new OutCodecCtx(ctx);
    }
    return nullptr;
}
VideoOutCodecCtx* VideoOutCodecCtx::findById(enum AVCodecID id) {
    auto ctx = CodecCtx::findById(id, true);
    if (ctx != nullptr) {
        return new VideoOutCodecCtx(ctx);
    }
    return nullptr;
}
OutCodecCtx::OutCodecCtx(AVCodecContext *ctx) : CodecCtx(ctx) {
    pkt = av_packet_alloc();
}
void rescale_pkt(AVPacket *pkt, AVRational tb_src, AVRational tb_dst)
{
    cout << "pts = " << pkt->pts
         << " dts = " << pkt->dts
         << " outPkgLen = " << pkt->size << endl;

    av_packet_rescale_ts(pkt,
                         tb_src,
                         tb_dst);
}
int OutCodecCtx::onFrame(CodecCtx *codecCtx,AVFrame *frame) {
    if (frame) {
        cout <<"frame pts = " <<frame->pts <<endl;
    }
    int ret = avcodec_send_frame(ctx,frame);
    if (ret < 0) {
        return ret;
    }
    do {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            ret = 0;
            break;
        }
        if (sink != nullptr) {
            rescale_pkt(pkt,codecCtx->ctx->time_base,stream->time_base);
            ret = sink->onPackage(pkt);
        }
        av_packet_unref(pkt);
    } while (ret >=0);
    return ret;
}

OutCodecCtx::~OutCodecCtx() {
    if (pkt) {
        av_packet_free(&pkt);
    }
}

void OutCodecCtx::setPacketSink(PacketSink *_sink) {
    sink = _sink;
}

SubTitle::SubTitle(AVCodecContext *_ctx) : OutCodecCtx(_ctx) {
    subtitle_out = nullptr;
    rect.type = SUBTITLE_ASS;
    rects = &rect;
    sub.rects = &rects;
    sub.format = 1;
    sub.num_rects = 1;
    text_buff = nullptr;
}

int SubTitle::open() const {
    if (ctx->subtitle_header == nullptr) {
        const char* header = av_asprintf("");
        ctx->subtitle_header  = (uint8_t*) header;
        ctx->subtitle_header_size = 0;
    }
    return CodecCtx::open();
}

SubTitle::~SubTitle() {
    if (av_codec_is_encoder(ctx->codec)) {
        av_free(ctx->subtitle_header);
    }
    if (subtitle_out) {
        av_free(subtitle_out);
    }
    if (text_buff) {
        av_free(text_buff);
    }
}

int SubTitle::setSize(size_t _size) {
    if (subtitle_out) {
        if (_size == size) {
            return 0;
        }
        av_free(subtitle_out);
    }
    if (text_buff) {
        av_free(text_buff);
    }
//    int subtitle_out_max_size = 1024 * 2;
    subtitle_out = (char *)av_malloc(_size);
    if (subtitle_out) {
        subtitle_out[0] = 0;
        this->size = _size;
    }
    text_buff = (char *)av_malloc(_size);
    if (text_buff) {
        text_buff[0] = 0;
    }
    return 0;
}

int SubTitle::encodeTxt(AVPacket* pkgRel,const char* subTile,int64_t duration) {
    sprintf(text_buff,"Dialogue: 0,0:00:00.00,0:00:00.00,Default,,0,0,0,%s",subTile);
    rect.ass = text_buff;
    int len = avcodec_encode_subtitle(ctx, (uint8_t*)subtitle_out,(int)size,&sub);
    rect.ass = nullptr;
    if (len ==0) {
        return len;
    }
    if (len < 0) {
        CodecCtx::printErr(len);
        return len;
    }
    pkt->size = len;
    pkt->data = (uint8_t*)subtitle_out;
    pkt->duration = duration;
    pkt->pts = pkgRel->pts;
    pkt->dts = pkgRel->dts;
    rescale_pkt(pkt,ctx->time_base,stream->time_base);
    if (sink != nullptr) {
        sink->onPackage(pkt);
    }
    return 0;
}

SubTitle *SubTitle::findById(enum AVCodecID id, bool encoder, size_t bufferSize) {
    auto *codec = CodecCtx::findById(id,encoder);
    if (codec) {
        auto *subT = new SubTitle(codec);
        subT->setSize(bufferSize);
        return subT;
    }
    return nullptr;
}

InCodecCtx::InCodecCtx(AVCodecContext *ctx) : CodecCtx(ctx) {
    frame = av_frame_alloc();
}

InCodecCtx::~InCodecCtx() {
    if (frame) {
        av_frame_free(&frame);
    }
}


void InCodecCtx::setFrameSink(FrameSink *_sink) {
    sink = _sink;
}
VideoOutCodecCtx::~VideoOutCodecCtx() {
    if (sws) {
        sws_freeContext(sws);
        sws = nullptr;
    }
    if (_frame) {
        av_frame_free(&_frame);
    }
}
VideoOutCodecCtx::VideoOutCodecCtx(AVCodecContext *_ctx) :sws(nullptr), OutCodecCtx(_ctx) {

}
int VideoOutCodecCtx::onFrame(CodecCtx *codecCtx,AVFrame *frame) {
    if (sws && frame) {
        av_frame_copy_props(this->_frame,frame);
        // int ret = sws_scale_frame(sws, this->_frame,frame);
        int ret = sws_scale(sws,(const uint8_t * const*)frame->data,frame->linesize,0,frame->height,_frame->data,_frame->linesize);
        if (ret < 0) {
            return ret;
        }
        ret = OutCodecCtx::onFrame(codecCtx,_frame);
        //av_frame_unref(_frame);
        return ret;
    } else {
        return OutCodecCtx::onFrame(codecCtx,frame);
    }
}

int VideoOutCodecCtx::initVideo(AVFrame *frame) {
    int ret = -1;
    const enum AVPixelFormat *pix_fmts_supports = ctx->codec->pix_fmts;
    if (pix_fmts_supports == nullptr) {
        return ret;
    }
    do{
        if (*pix_fmts_supports == frame->format) {
            break;
        }
        pix_fmts_supports++;
    } while(*pix_fmts_supports  != AV_PIX_FMT_NONE);

    if (*pix_fmts_supports == AV_PIX_FMT_YUVJ444P || *pix_fmts_supports == AV_PIX_FMT_YUVJ422P) {
        pix_fmts_supports = nullptr;
    }

    // 需要转码
    if (pix_fmts_supports == nullptr || *pix_fmts_supports == AV_PIX_FMT_NONE) {
        enum AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
        ctx->width = frame->width;
        ctx->height = frame->height;

        if (ctx->width % 2 !=0) {
            ctx->width-=1;
        }
        if (ctx->height % 2 !=0) {
            ctx->height-=1;
        }
        sws = sws_getContext(
                frame->width, frame->height, (enum AVPixelFormat)frame->format,
                ctx->width, ctx->height, pix_fmt,
                SWS_FAST_BILINEAR, NULL, NULL, NULL);

        if (sws == nullptr) {
            cout << "sws_getContext fail" << endl;
            ret = -1;
        } else {
            ret = 0;
        }
        _frame = av_frame_alloc();
        _frame->format = pix_fmt;
        _frame->width = ctx->width;
        _frame->height = ctx->height;
        ret = av_frame_get_buffer(_frame, 1);
        ctx->pix_fmt = pix_fmt;
    } else {
        ctx->width = frame->width;
        ctx->height = frame->height;
        ctx->pix_fmt = *pix_fmts_supports;
    }

    return ret;
}
