//
// Created by micro on 2023/8/21.
//

#include "UdpCodec.h"
#include <iostream>
#include "crc16.h"
using namespace std;
void UdpCodec::onPackage(const char *buff, int len) {
    if (len < 4) {
        return;
    }

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
        if (flag == 1 && this->pos >0 && listener) {
            listener->onImage(this->jpg_buff,this->pos);
            pktId = 0;
            pos = 0;
        }
    }


}
UdpCodec::UdpCodec(int buff_size): buff_size(buff_size) {
    this->jpg_buff = (char*)malloc(buff_size);
}

UdpCodec::~UdpCodec() {
    this->release();
    if (jpg_buff) {
        free(jpg_buff);
        jpg_buff = nullptr;
    }
}

void UdpCodec::release() {

}


void UdpCodec::setJpgListener(JpgListener *_listener) {
    listener = _listener;
}

