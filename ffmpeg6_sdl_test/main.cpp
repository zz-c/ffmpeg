// main.cpp: 定义应用程序的入口点。
//

#pragma once

#include <iostream>
#include<SDL.h>
#include "Test.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

using namespace std;

int main(int argc, char* argv[])
{
    //初始化SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        SDL_Log("can not init SDL:%s", SDL_GetError());
        return -1;
    }
    SDL_Window* window = SDL_CreateWindow("SDL SDL_FFMPEG", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    SDL_Renderer* render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(render, 0, 255, 255, 255);
    SDL_Rect rect = { 200, 300, 100, 100 };
    SDL_RenderDrawRect(render, &rect);
    SDL_RenderPresent(render);
    
    Test* test = new Test(render,800,600);
    test->testRtsp();
    //getchar();

    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();

}
