//
// Created by micro on 2023/8/21.
//

#include "UdpServer.h"
#ifdef __WINNT

#include <winsock2.h>
#include <Ws2tcpip.h>
#endif
#include <iostream>
#include "crc16.h"
using namespace std;

UdpServer::UdpServer(int buffer_size) :buffer_size(buffer_size) {
    buff = (char*)malloc(buffer_size);
    s = 0;
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
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) {
        ret = WSAGetLastError();
        cout << "Could not create socket : " <<  ret << endl;
        this->release();
        return ret;
    }

    // Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // Bind
    if (bind(s, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        ret = WSAGetLastError();
        cout <<"Bind failed with error code : " << ret << endl;
        this->release();
        return ret;
    }
    ret = 0;
#endif
    return ret;
}
UdpServer::~UdpServer() {
    this->release();
    free(buff);
}

void UdpServer::release() {
#ifdef __WINNT
    if (s) {
        closesocket(s);
        s = 0;
    }
    WSACleanup();
#endif
}
void UdpServer::loop() {
    int recv_len;
    int fLen = sizeof(remote);
    for(;;) {
        if ((recv_len = recvfrom(s, buff, buffer_size, 0, (struct sockaddr*)&remote, &fLen)) == SOCKET_ERROR) {
//            cout <<"recvfrom failed with error code : " << WSAGetLastError() << endl;
            break;
        }
        if (l) {
            l->onPackage(buff,recv_len);
        }
    }
}

void UdpServer::setListener(UdpListener *_l) {
    this->l = _l;
}
