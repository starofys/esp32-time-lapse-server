#include <iostream>
#include <fstream>
#include "demo1.h"
#include "CodecCtx.h"
#include "FormatCtx.h"
#include "UdpServer.h"
#include <csignal>
#include "CaptureApp.h"
#include "cmdline.h"

using namespace std;
UdpServer server(1500);
UdpCodec udpCodec;
void SignalHandler(int signal)
{
    cout << "shutdown " << signal << endl;
    if (signal == SIGINT || signal ==SIGTERM ) {
        server.release();
    }
}
cmdline::parser p;
JpgListener* create() {
    auto capApp = new CaptureApp;
    if (capApp->setBufferSize(200000)) {
        auto encoder = p.get<string>("encoder");
        auto format = p.get<string>("format");
        auto rate = p.get<int>("rate");
        auto webvtt = p.get<bool>("webvtt");
        if (capApp->initParams(encoder, format, rate, webvtt) >= 0) {
            return capApp;
        }
    }
    delete capApp;
    return nullptr;
}

int main (int argc,char*argv[])
{
    p.set_program_name(argv[0]);
    p.add<string>("encoder",'e',"encoder name",false,"libx264");
    p.add<int>("port",'p',"listen port",false,8080,cmdline::range(1,65535));
    p.add<string>("format",'f',"out format",true,"mp4");
    p.add<bool>("webvtt",'w',"enable single webvtt subtitle file output",false, false);
    p.add<int>("rate",'r',"video rate",false,25,cmdline::range(1,60));
    p.add<int>("timeout",'t',"clean instance timeout minute",false,30,cmdline::range(1,60*24));
    p.add<int>("limit",'l',"max capture size",false,2,cmdline::range(1,10));
    p.parse_check(argc,argv);

    auto val = create();
    if (!val) {
        return -1;
    }
    delete val;

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    udpCodec.timeout = p.get<int>("timeout") * 60;
    udpCodec.limit = p.get<int>("limit");
    udpCodec.setJpgListener(create);
    server.setListener(&udpCodec);

    int ret;
    ret = server.open(p.get<int>("port"));
    if (ret !=0) {
        return ret;
    }
    server.loop();
    server.setListener(nullptr);
    udpCodec.release();
    cout << "finished" << endl;
    return ret;

}

