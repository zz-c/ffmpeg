#pragma once
#include <iostream>
#include<SDL.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class Test {
public:
    Test(SDL_Renderer* render, int w, int h):render(render), w(w), h(h){
        std::cout << "construct Test..." << std::endl;
    };

    ~Test() {
        std::cout << "destruct Test..." << std::endl;
        if (pFormatCtx != nullptr) {
            avformat_free_context(pFormatCtx);
            pFormatCtx = nullptr;
        }
        if (pCodecCtx != nullptr) {
            avcodec_free_context(&pCodecCtx);
            pCodecCtx = nullptr;
        }
        if (pPacket != nullptr) {
            av_packet_free(&pPacket);
            pPacket = nullptr;
        }
        if (pFrame != nullptr) {
            av_frame_free(&pFrame);
            pFrame = nullptr;
        }
    }
public:
    int testRtsp();
    AVFormatContext* pFormatCtx = nullptr;
    AVCodecContext* pCodecCtx = nullptr;
    AVPacket* pPacket = nullptr;
    AVFrame* pFrame = nullptr;
private:
    int h;
    int w;
    SDL_Renderer* render = nullptr;
};