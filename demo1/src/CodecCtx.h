//
// Created by micro on 2023/8/12.
//

#ifndef CPP_PROJECTS_CODECCTX_H
#define CPP_PROJECTS_CODECCTX_H

#include <string>
#include "demo1.h"
class CodecCtx {
public:
    static void printErr(int ret,const char* msg = "");
    static AVCodecContext* findById(enum AVCodecID id,bool encoder);
    static AVCodecContext* findByName(const char* name,bool encoder);
    AVCodecContext *ctx;
    AVStream *stream;
    explicit CodecCtx(AVCodecContext *_ctx);
    virtual ~CodecCtx();
    virtual int open() const;
};

class FrameSink {
public:
    virtual int onFrame(CodecCtx *codecCtx,AVFrame* frame) = 0;
};
class PacketSink {
public:
    virtual int onPackage(AVPacket* pkg) = 0;
};

class OutCodecCtx : public CodecCtx, public FrameSink {
protected:
    PacketSink *sink = nullptr;
public:
    static OutCodecCtx* findById(enum AVCodecID id);
    AVPacket *pkt;
    explicit OutCodecCtx(AVCodecContext *ctx);
    ~OutCodecCtx() override;
    void setPacketSink(PacketSink *sink);
    int onFrame(CodecCtx *codecCtx,AVFrame *frame) override;
};

class VideoOutCodecCtx :public OutCodecCtx {
private:
    struct SwsContext *sws = nullptr;
    AVBufferRef *hw_device_ctx = nullptr;
public:
    static VideoOutCodecCtx* findById(enum AVCodecID id);
    static VideoOutCodecCtx* findByName(const char* name);
    explicit VideoOutCodecCtx(AVCodecContext *_ctx);
    ~VideoOutCodecCtx() override;
    int initDefault(int rate);
    int initVideo(AVFrame *frame);
    int onFrame(CodecCtx *codecCtx,AVFrame *frame) override;
};

class InCodecCtx : public CodecCtx, public PacketSink {
protected:
    FrameSink *sink = nullptr;
public:
    static InCodecCtx* findById(enum AVCodecID id);
    AVFrame *frame;
    explicit InCodecCtx(AVCodecContext *ctx);
    ~InCodecCtx() override;
    void setFrameSink(FrameSink *sink);
    int onPackage(AVPacket *pkg) override;
};

class SubTitle : public OutCodecCtx {
private:
    AVSubtitleRect *rects;
public:
    static SubTitle* findById(enum AVCodecID id,bool encoder,size_t bufferSize);
    AVSubtitle sub = {0};
    AVSubtitleRect rect = {0};
    explicit SubTitle(AVCodecContext *_ctx);
    ~SubTitle() override;
    int setSize(size_t size);
    int open() const override;
    int encodeTxt(AVFrame* frame,const char* subTile);
private:
    char *subtitle_out;
    size_t size = 0 ;
    char *text_buff;
};


#endif //CPP_PROJECTS_CODECCTX_H
