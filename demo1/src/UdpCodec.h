//
// Created by micro on 2023/8/21.
//

#ifndef CPP_PROJECTS_UDPCODEC_H
#define CPP_PROJECTS_UDPCODEC_H

#include <map>
#include <functional>
#include <string>
#include "UdpServer.h"
#include <cstdint>
using namespace std;
enum CaptureStatus {
    CAP_INIT = 0,
    CAP_RECV_IMG
};

class JpgListener {
private:
    char* jpg_buff = nullptr;
    int buff_size = 0;
    int pos = 0;
    int pktId = 0;
public:
    time_t currentTim  = 0;
    virtual ~JpgListener();
    virtual int onImage(const char *buff, int len) = 0;
    virtual int valid() = 0;
    int setBufferSize(int buff_size);
    void onPackage(const char* buff,int len);
};

class UdpCodec : public UdpListener{
private:
    map<std::string,JpgListener*> listeners;
    function<JpgListener*()> factory;
    unsigned long id;
public:
    int timeout;
    int limit;
    UdpCodec();
    ~UdpCodec();
    void onPackage(struct sockaddr_storage *remote,const char* buff,int len) override;
    void release();
    void setJpgListener(function<JpgListener*()> f);
    void clean(time_t now);
};


#endif //CPP_PROJECTS_UDPCODEC_H
