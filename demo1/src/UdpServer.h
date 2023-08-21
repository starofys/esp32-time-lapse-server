//
// Created by micro on 2023/8/21.
//

#ifndef CPP_PROJECTS_UDPSERVER_H
#define CPP_PROJECTS_UDPSERVER_H

#ifdef __WINNT
#include <winsock.h>
#include <windows.h>
#endif

class UdpListener {
public:
    virtual void onPackage(const char* buff,int len) = 0;
};

class UdpServer {
private:
    char *buff;
    const int buffer_size;
    UdpListener* l = nullptr;
#ifdef __WINNT
    struct sockaddr_in server = {0};
    struct sockaddr_in remote = {0};
    SOCKET s;
#endif
public:
    UdpServer(int buffer_size);
    ~UdpServer();
    int open(unsigned short port);
    void release();
    void loop();
    void setListener(UdpListener* l);
};


#endif //CPP_PROJECTS_UDPSERVER_H
