//
// Created by micro on 2023/8/12.
//
#ifndef CPP_PROJECTS_FORMATCTX_H
#define CPP_PROJECTS_FORMATCTX_H

#include "demo1.h"
#include "CodecCtx.h"
#include <vector>

class FormatCtx {
public:
    AVFormatContext *fmt = nullptr;
    // 记录文件输入的pts
    int pts = 0;
    FormatCtx();
    ~FormatCtx();
    int open() const;
    virtual int initBy(const char *filename);
    virtual int close();
    void dumpFmt();
protected:
    std::vector<CodecCtx*> codecs;
};

class FormatOutput : public FormatCtx, public PacketSink {
private:
public:
    FormatOutput();
    ~FormatOutput();
    int initBy(const char *filename) override;
    int onPackage(AVPacket* pkg) override;
    int addStream(CodecCtx *ctx);
    int close() override;
};

#endif //CPP_PROJECTS_FORMATCTX_H
