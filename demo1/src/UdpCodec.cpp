//
// Created by micro on 2023/8/21.
//

#include "UdpCodec.h"
#include <iostream>
#include "crc16.h"
#include <cstring>
#include <utility>
#ifndef __WINNT
#include <arpa/inet.h>
#endif

using namespace std;
void UdpCodec::onPackage(struct sockaddr_storage *remote,const char *buff, int len) {
    if (len < 4) {
        return;
    }
    std::string ip;

    char str[INET6_ADDRSTRLEN];

    if (remote->ss_family == AF_INET) {
        auto in = (sockaddr_in*)remote;
        ip = inet_ntop(remote->ss_family, &in->sin_addr, str,INET6_ADDRSTRLEN);
        ip += ":";
        ip += std::to_string(in->sin_port);
    } else if (remote->ss_family == AF_INET6){
        auto in = (sockaddr_in6*)remote;
        ip = inet_ntop(remote->ss_family,  &in->sin6_addr, str,INET6_ADDRSTRLEN);
        ip += ":";
        ip += std::to_string(in->sin6_port);
    }
    time_t now = time(nullptr);
    auto itr  = listeners.find(ip);
    JpgListener *listener;
    if (itr == listeners.end()) {
        if (listeners.size() < limit) {
            cout << "new ip capture " << ip.c_str() << endl;
            listener = factory();
            if (listener) {
                listeners[ip] = listener;
            }
        } else {
            listener = nullptr;
            clean(now);
        }
    } else {
        listener = itr->second;
    }
    if (listener) {
        listener->currentTim = now;
        listener->onPackage(buff, len);
        if (listener->valid() < 0 && itr != listeners.end()) {
            cout << "listener not valid remove it " << listener->valid() << endl;
            listeners.erase(itr);
            delete listener;
        }
    }
    id++;
    if (id % 1000 == 0) {
        this->clean(now);
        id = 0;
    }
}


UdpCodec::~UdpCodec() {
    this->release();
}

void UdpCodec::release() {
    for (const auto &item: listeners)  {
        delete item.second;
    }
    listeners.clear();
}

void UdpCodec::setJpgListener(function<JpgListener *()> f) {
    factory = std::move(f);
}

UdpCodec::UdpCodec() {

}

void UdpCodec::clean(time_t now) {
    auto itr = listeners.begin();
    while (itr != listeners.end()) {
        double second = difftime(now, itr->second->currentTim);
        if (second > timeout) {
            delete itr->second;
            itr = listeners.erase(itr);
        } else {
            itr++;
        }
    }
}


int JpgListener::setBufferSize(int size) {
    if (this->jpg_buff) {
        free(this->jpg_buff);
        this->buff_size = 0;
    }
    this->jpg_buff = (char*)malloc(size);
    if (this->jpg_buff) {
        this->buff_size = size;
    } else {
        return -1;
    }
    return this->buff_size;
}

JpgListener::~JpgListener() {
    if (jpg_buff) {
        free(jpg_buff);
        jpg_buff = nullptr;
    }
}

void JpgListener::onPackage(const char *buff, int len) {
// 4bit cmd 4bit flag 12bit idx 12bit crc16
    int head = *(int*)buff;
    unsigned short crc16 = head & 0xfff;
    buff=&buff[4];
    len = len - 4;
    unsigned short cal = CRC16(buff,len) & 0xfff;
    if (cal != crc16) {
        cout << "crc16 error !" <<endl;
        return;
    }
    int cmd = head >> 28;
    int flag = head >> 24 & 0xf;
    int idx = head >> 12 & 0xfff;

    // cout <<"cmd=" << cmd << " flag= " <<  flag << " len=" <<  len  <<  " idx=" << idx << endl;

    // 图片传输
    if (cmd == 1) {
        if (flag == 0 || flag == 1) {
            if (pktId == idx) {
                if (pos + len > buff_size) {
                    this->jpg_buff = (char*)realloc(this->jpg_buff,pos + len);
                    buff_size = pos + len;
                }
                memcpy((void *)&jpg_buff[pos],(void *)buff,len);
                pos+=len;
                pktId++;
            } else {
                pktId = 0;
                pos = 0;
            }
        }
        // 结束包
        if (flag == 1 && this->pos >0) {
            onImage(this->jpg_buff,this->pos);
            pktId = 0;
            pos = 0;
        }
    }
}
