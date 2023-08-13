//
// Created by micro on 2023/8/12.
//

#ifndef CPP_PROJECTS_FORMATCTX_H
#define CPP_PROJECTS_FORMATCTX_H

#include "demo1.h"
#include "CodecCtx.h"

class FormatCtx {
private:
public:
    AVFormatContext *fmt;
    int is_output;
    const char* filename;
    FormatCtx();
    ~FormatCtx();
    int newOutput(const char* filename);
    int open();
    int newStream(CodecCtx *ctx);
    void dumpFmt();
};

#endif //CPP_PROJECTS_FORMATCTX_H
