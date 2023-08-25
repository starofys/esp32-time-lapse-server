//
// Created by micro on 2023/8/21.
//

#include "UdpServer.h"
#include <iostream>

#include "crc16.h"
using namespace std;

UdpServer::UdpServer(int buffer_size) :buffer_size(buffer_size) {
    buff = (char*)malloc(buffer_size);
    fd = 0;
}

int UdpServer::open(unsigned short port) {
    int ret = -1;
#ifdef __WINNT
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        ret = WSAGetLastError();
        cout << "Failed. Error Code : " << ret <<endl;
        return ret;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET) {
        ret = WSAGetLastError();
        cout << "Could not create socket : " <<  ret << endl;
        this->release();
        return ret;
    }
#else
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        this->release();
        return fd;
    }
#endif
    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    ret = bind(fd, (struct sockaddr*)&server, sizeof(server));
    if (ret <0) {
#ifdef __WINNT
        ret = WSAGetLastError();
#endif
        cout <<"Bind failed with error code : " << ret << endl;
        this->release();
    } else {
        ret = 0;
    }
    return ret;
}
UdpServer::~UdpServer() {
    this->release();
    free(buff);
}

void UdpServer::release() {
#ifdef __WINNT
    if (fd) {
        closesocket(fd);
        fd = 0;
    }
    WSACleanup();
#else
    close(fd);
#endif
}
void UdpServer::loop() {
    int recv_len;
    socklen_t fLen = sizeof(remote);
//    for(int i=0;i<4000;i++) {
     for(;;) {
        if ((recv_len = recvfrom(fd, buff, buffer_size, 0, (struct sockaddr*)&remote, &fLen)) < 0) {
//            cout <<"recvfrom failed with error code : " << WSAGetLastError() << endl;
            break;
        }
        if (l) {
            l->onPackage(buff,recv_len);
        }
    }
    cout << "udp break" << endl;
}

void UdpServer::setListener(UdpListener *_l) {
    this->l = _l;
}
