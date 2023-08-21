#include "demo1.h"
#include <iostream>
#include <fstream>
using namespace std;
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

int main2() {
    const char *outfile = "output.mp4";
    int rate = 25;
    int ret;
    AVRational sourceRate = av_make_q(1, rate);
    // 读取图片
    int num = 2;
    AVPacket *pkgList[num];
    for (int i = 0; i < num; i++) {
        string name = "H:/";
        name += std::to_string(i);
        name += ".jpg";
        pkgList[i] = readAll(name.c_str());
    }
}