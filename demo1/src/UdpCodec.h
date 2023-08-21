//
// Created by micro on 2023/8/21.
//

#ifndef CPP_PROJECTS_UDPCODEC_H
#define CPP_PROJECTS_UDPCODEC_H
#include "UdpServer.h"
enum CaptureStatus {
    CAP_INIT = 0,
    CAP_RECV_IMG
};

class JpgListener {
public:
    virtual void onImage(const char *buff, int len) = 0;
};

class UdpCodec : public UdpListener{
private:
    char* jpg_buff;
    int buff_size;
    int pos = 0;
    int pktId = 0;
    CaptureStatus status;
    JpgListener *listener = nullptr;
public:
    UdpCodec(int buff_size);
    ~UdpCodec();
    void onPackage(const char* buff,int len) override;

    void release();
    void setJpgListener(JpgListener *listener);
};


#endif //CPP_PROJECTS_UDPCODEC_H
