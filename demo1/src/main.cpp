#include <iostream>
#include <fstream>
#include "demo1.h"
#include "CodecCtx.h"
#include "FormatCtx.h"
using namespace std;
void printErr(int ret) {
    char a[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    av_make_error_string(a, AV_ERROR_MAX_STRING_SIZE, ret);
    cout << a << endl;
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
}
AVPacket* readAll(const char* filename) {
    cout << filename <<endl;
    ifstream file;
    file.open(filename,ios::in|ios::binary);
    if (!file.is_open()) {
        return nullptr;
    }
    file.seekg(0, std::ios::end);
    int fileLength = file.tellg();
    //指定定位到文件开始
    file.seekg(0, std::ios::beg);
    cout << "fileLength:" << fileLength << endl;
    auto * buffer = new uint8_t[fileLength + 1];
    file.read((char*)buffer, fileLength);
    AVPacket *pkg = av_packet_alloc();
    av_packet_from_data(pkg,buffer,fileLength);
    return pkg;
}

int main3() {

    int ret = -1;
//    const char* filename = "H:\\test.jpg";
//    cout << filename <<endl;
//    ifstream file;
//    file.open(filename,ios::in|ios::binary);
//    if (!file.is_open()) {
//        return -1;
//    }
//    file.seekg(0, std::ios::end);
//    int fileLength = file.tellg();
//
//    //指定定位到文件开始
//    file.seekg(0, std::ios::beg);
//
//    cout << "fileLength:" << fileLength << endl;
//    auto * buffer = new uint8_t[fileLength + 1];
//    file.read((char*)buffer, fileLength);
//
//    struct SwsContext *sws_ctx = nullptr;
//
//    AVPacket *pkg = av_packet_alloc();
//    ret = av_packet_from_data(pkg,buffer,fileLength);
//
//    if (ret < 0) {
//        return ret;
//    }

    const AVCodec *vCodec;
    AVCodecContext *vCodecCtx;
    vCodec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
    if (!vCodec) {
        ret = -1;
        return ret;
    }
    vCodecCtx =  avcodec_alloc_context3(vCodec);
    if (!vCodecCtx) {
        ret = -1;
        return ret;
    }
//    logFormat(vCodec);
//    vCodecCtx->time_base = av_make_q(25,1);
    //vCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    ret = avcodec_open2(vCodecCtx,vCodecCtx->codec, nullptr);

    if (ret < 0) {
        printErr(ret);
        return ret;
    }

//


//    AVFrame *frame = av_frame_alloc();
//    for (int i = 0; i < 50; ++i) {
//        ret = avcodec_send_packet(vCodecCtx,pkg);
//        if (ret < 0) {
//            printErr(ret);
//            return ret;
//        }
//        ret = avcodec_receive_frame(vCodecCtx,frame);
//        if (ret == 0) {
//            if (!sws_ctx) {
//                const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(frame->format));
//                cout << desc->name << endl;
//                sws_ctx = sws_getContext(
//                        frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
//                        frame->width, frame->height, AV_PIX_FMT_YUV420P,
//                        SWS_BILINEAR, NULL, NULL, NULL);
//            }
//            cout << "received" << endl;
//        }
//    }

    return ret;

}

int encode(CodecCtx &outCtx,AVPacket *outPkg,AVRational src,AVFormatContext* fmt,AVFrame *frame){
    int ret;
    ret = avcodec_send_frame(outCtx.ctx,frame);
    if (ret < 0) {
        return ret;
    }
    if (frame) {
        cout <<"frame pts = " <<frame->pts <<endl;
    }
    do {
        ret = avcodec_receive_packet(outCtx.ctx,outPkg);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            ret = 0;
            break;
        }
//        int64_t old =outPkg->pts;
        if (ret == 0) {
            av_packet_rescale_ts(outPkg,
                                 src,
                                 outCtx.stream->time_base);
            cout <<"pts = " <<outPkg->pts
            << " dts = " << outPkg->dts
            << " frame pts = "<<((frame != nullptr)?frame->pts : 0)<< " outPkgLen = "<< outPkg->size  << endl;

//            if (outPkg->dts < 0) {
//                outPkg->dts = 0;
//            }

            ret = av_interleaved_write_frame(fmt,outPkg);
        }
        av_packet_unref(outPkg);
    } while (ret >= 0);

    return ret;
}
int initOut(int width,int height,int rate,enum AVPixelFormat pix_fmt,CodecCtx &outCtx,FormatCtx &formatCtx) {
    int ret;
    outCtx.initVideo(width,height,rate,pix_fmt);

    ret = outCtx.open();
    if (ret < 0) {
        return ret;
    }
    ret = formatCtx.newStream(&outCtx);
    if (ret < 0) {
        return ret;
    }
    ret = formatCtx.open();
    return ret;
}
int main() {
    const char* outfile = "output.mp4";
    int ret;

    CodecCtx inCtx(AV_CODEC_ID_MJPEG, false);
    CodecCtx outCtx(AV_CODEC_ID_H264, true);

    struct SwsContext* sws_ctx;

//    logFormat(inCtx.codec);
//    logFormat(outCtx.codec);


    ret = inCtx.open();
    if (ret < 0) {
        return ret;
    }



    FormatCtx formatCtx;
    formatCtx.newOutput(outfile);


    int rate = 30;
    AVRational src =  av_make_q(1,rate);

    AVFrame *frame = av_frame_alloc();
    AVPacket *outPkg = av_packet_alloc();
    AVFrame *dist = av_frame_alloc();

    int num = 2;

    AVPacket* pkgList[num];

    for (int i =0;i<num;i++) {
        string name = "H:/";
        name += std::to_string(i);
        name+=".jpg";
        pkgList[i] = readAll(name.c_str());
    }


    bool init = false;
    int64_t pts = 1000;
    int index = 0;
    for (int i = 0; i < rate * 5; ++i) {
        AVPacket* pkg = pkgList[index];

        pkg->pts = pts;
        pkg->dts = pts;

//        frame->pts = pts;
//        frame->pkt_dts = pts-1;

        pts++;

        if (i % rate ==0) {
            index++;
            if (index == num) {
                index = 0;
            }
        }

        if (i % rate !=0) {
            continue;
        }




        ret = avcodec_send_packet(inCtx.ctx,pkg);
        if (ret < 0) {
            printErr(ret);
            return ret;
        }
        do {
            ret = avcodec_receive_frame(inCtx.ctx,frame);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                ret = 0;
                break;
            }
            if (!init) {
                init = true;

                int width = frame->width;
                int height = frame->height;
                if (width % 2 !=0) {
                    width-=1;
                }
                if (height % 2 !=0) {
                    height-=1;
                }
                ret = initOut(width,height,rate,AV_PIX_FMT_YUV420P,outCtx,formatCtx);
                if (ret < 0) {
                    printErr(ret);
                    return ret;
                }
                sws_ctx = sws_getContext(
                        frame->width, frame->height, (AVPixelFormat)frame->format,
                        width, height, AV_PIX_FMT_YUV420P,
                        SWS_FAST_BILINEAR, NULL, NULL, NULL);
            }

            ret = sws_scale_frame(sws_ctx,dist,frame);
            if (ret < 0) {
                printErr(ret);
                return ret;
            }
            dist->format = AV_PIX_FMT_YUV420P;
            dist->pts = frame->pts;
            dist->pkt_dts = frame->pkt_dts;
            dist->duration = frame->duration;

            ret = encode(outCtx,outPkg,src,formatCtx.fmt,dist);
        } while (ret >=0);
    }

    ret = encode(outCtx, outPkg,src,formatCtx.fmt, nullptr);

    av_write_trailer(formatCtx.fmt);

    return ret;

}

