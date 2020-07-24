#pragma once
#include "OtherRender.h"
class YUVRender: public OtherRender
{
public:
    YUVRender();
    ~YUVRender();
    virtual int CheckTextrue(int* width, int* height, SDL_PixelFormatEnum* pixfmt);
    //刷新画板，0为成功，其余失败
    virtual int FlushTexture(SDL_Texture *pSdlPainter);
private:
    AVPixelFormat m_pfPixfmt;
};

