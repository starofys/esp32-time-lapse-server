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

void CodecCtx::printErr(int ret,const char* msg) {
    char a[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    av_make_error_string(a, AV_ERROR_MAX_STRING_SIZE, ret);
    cout << a << msg << endl;
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
AVCodecContext* CodecCtx::findByName(const char* name, bool encoder) {
    const AVCodec *codec;
    if (encoder) {
        codec = avcodec_find_encoder_by_name(name);
    } else {
        codec = avcodec_find_encoder_by_name(name);
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
        CodecCtx::printErr(ret," send pkt err");
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
VideoOutCodecCtx *VideoOutCodecCtx::findByName(const char *name) {
    auto ctx = CodecCtx::findByName(name, true);
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
    cout << "in pts = " << pkt->pts  << " dts = " << pkt->dts << " outPkgLen = " << pkt->size << endl;
    av_packet_rescale_ts(pkt, tb_src,  tb_dst);
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
        ctx->subtitle_header_size = strlen(header);
    }
    return CodecCtx::open();
}

SubTitle::~SubTitle() {
    if (av_codec_is_encoder(ctx->codec)) {
        av_free(ctx->subtitle_header);
        ctx->subtitle_header = nullptr;
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

int SubTitle::encodeTxt(AVFrame* frame,const char* subTile) {
    sprintf(text_buff,"Dialogue: 0,0:00:00.00,0:00:00.00,Default,,0,0,0,%s",subTile);
    rect.ass = text_buff;
    int len = avcodec_encode_subtitle(ctx, (uint8_t*)subtitle_out,(int)size,&sub);
    rect.ass = nullptr;
    if (len ==0) {
        cout << "subTitle len = " << len << endl;
        return len;
    }
    if (len < 0) {
        cout << "2 subTitle len = " << len << endl;
        CodecCtx::printErr(len);
        return len;
    }
    pkt->stream_index = stream->index;
    pkt->size = len;
    pkt->data = (uint8_t*)subtitle_out;

    pkt->pts = frame->pts;
    pkt->dts = frame->pkt_dts;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 2, 100)
    pkt->duration = frame->duration;
#else
    pkt->duration = frame->pkt_duration;
#endif
    av_packet_rescale_ts( pkt, ctx->time_base,  stream->time_base);

    cout << "3 subTitle pts = " << pkt->pts << endl;

    if (sink) {
        cout << "3 subTitle onPackage = " << pkt->pts << endl;
        int ret = sink->onPackage(pkt);
        cout << "5 subTitle onPackage rs =  " << ret << endl;
    } else {
        cout << "4 subTitle sink null = " << pkt->pts << endl;
    }
    av_packet_unref(pkt);
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
    if (hw_device_ctx) {
        av_buffer_unref(&hw_device_ctx);
    }
}
VideoOutCodecCtx::VideoOutCodecCtx(AVCodecContext *_ctx) :sws(nullptr), OutCodecCtx(_ctx) {

}
int VideoOutCodecCtx::onFrame(CodecCtx *codecCtx,AVFrame *frame) {
    if (sws && frame) {
        AVFrame *sw_frame = av_frame_alloc();
        if (ctx->hw_frames_ctx) {
            auto* hw_ctx = (AVHWFramesContext *)(ctx->hw_frames_ctx->data);
            sw_frame->format = hw_ctx->sw_format;
        } else {
            sw_frame->format = ctx->pix_fmt;
        }
        sw_frame->width = ctx->width;
        sw_frame->height = ctx->height;
        int ret = av_frame_get_buffer(sw_frame, 1);
        if (ret < 0) {
            CodecCtx::printErr(ret," get buffer err");
            return ret;
        }
        // int ret = sws_scale_frame(sws, this->_frame,frame);
        ret = sws_scale(sws,
                        (const uint8_t * const*)frame->data,
                        frame->linesize,0,frame->height,sw_frame->data,sw_frame->linesize);
        if (ret < 0) {
            CodecCtx::printErr(ret," sws_scale err");
            av_frame_free(&sw_frame);
            return ret;
        }
        if (ctx->hw_frames_ctx) {
            AVFrame * hw_frame = av_frame_alloc();
            ret = av_hwframe_get_buffer(ctx->hw_frames_ctx, hw_frame, 0);
            if (ret >= 0) {
                ret = av_hwframe_transfer_data(hw_frame, sw_frame, 0);
                if (ret < 0) {
                    CodecCtx::printErr(ret," hwframe transfer err");
                } else {
                    av_frame_copy_props(hw_frame,frame);
                    ret = OutCodecCtx::onFrame(codecCtx,hw_frame);
                }
            } else {
                CodecCtx::printErr(ret," hwframe get buffer err");
            }
            av_frame_free(&hw_frame);
        } else {
            av_frame_copy_props(sw_frame,frame);
            ret = OutCodecCtx::onFrame(codecCtx,sw_frame);
        }
        av_frame_free(&sw_frame);
        return ret;
    } else {
        return OutCodecCtx::onFrame(codecCtx,frame);
    }
}
static int set_hwframe_ctx(AVCodecContext *ctx, AVBufferRef *hw_device_ctx,int width,int height)
{
    AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames_ctx = NULL;
    int err = 0;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        fprintf(stderr, "Failed to create VAAPI frame context.\n");
        return -1;
    }
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width     = width;
    frames_ctx->height    = height;
    frames_ctx->initial_pool_size = 20;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        CodecCtx::printErr(err);
        cout << "Failed to initialize VAAPI frame context."
                        "Error code: " << err << endl;
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx)
        err = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return err;
}
int VideoOutCodecCtx::initVideo(AVFrame *frame) {
    int ret = -1;
    const enum AVPixelFormat *pix_fmts_supports = ctx->codec->pix_fmts;
    if (pix_fmts_supports == nullptr) {
        CodecCtx::printErr(ret," pix_fmts");
        return ret;
    }
    do{
        if (*pix_fmts_supports == frame->format) {
            break;
        }
        pix_fmts_supports++;
    } while(*pix_fmts_supports  != AV_PIX_FMT_NONE);

    // jpeg需要转yuv420
    if (*pix_fmts_supports == AV_PIX_FMT_YUVJ444P || *pix_fmts_supports == AV_PIX_FMT_YUVJ422P) {
        pix_fmts_supports = nullptr;
    }

    // 需要转码
    if (pix_fmts_supports == nullptr || *pix_fmts_supports == AV_PIX_FMT_NONE) {
        enum AVPixelFormat pix_fmt = ctx->codec->pix_fmts[0];
        ctx->width = frame->width;
        ctx->height = frame->height;

        if (ctx->width % 2 !=0) {
            ctx->width-=1;
        }
        if (ctx->height % 2 !=0) {
            ctx->height-=1;
        }
        ctx->pix_fmt = pix_fmt;
        if (pix_fmt == AV_PIX_FMT_VAAPI) {
            ret = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI,
                                         NULL, NULL, 0);
            if (ret >= 0) {
                ret = set_hwframe_ctx(ctx, hw_device_ctx,ctx->width,ctx->height);
            }
            if (ret >=0) {
                cout <<"set_hwframe_ctx success !" << endl;
            } else {
                CodecCtx::printErr(ret);
                cout <<"Failed to create a VAAPI device" << endl;
                if (hw_device_ctx) {
                    av_buffer_unref(&hw_device_ctx);
                }
                hw_device_ctx = nullptr;
                return ret;
            }
            pix_fmt = AV_PIX_FMT_NV12;
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
    } else {
        ctx->width = frame->width;
        ctx->height = frame->height;
        ctx->pix_fmt = *pix_fmts_supports;
    }

    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(ctx->pix_fmt);
    cout << "pix_fmt = " << desc->name << endl;

    return ret;
}

int VideoOutCodecCtx::initDefault(int rate) {
    ctx->time_base = av_make_q(1, rate);
    //设置GOP大小，即连续B帧最大数目
    ctx->gop_size= 30;
    ctx->bit_rate = 800000;
    ctx->max_b_frames = 16;
//    ctx->qcompress = 1.0;
    ctx->framerate = av_inv_q(ctx->time_base);

    if(ctx->codec->id == AV_CODEC_ID_H264) {
        av_opt_set(ctx->priv_data, "preset", "slow", 0);
    }
    //设置量化系数
    ctx->qmin=10;
    ctx->qmax=51;
    return 0;
}

