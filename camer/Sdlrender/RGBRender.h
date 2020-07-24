#pragma once
#include "OtherRender.h"
class RGBRender: public OtherRender
{
public:
    RGBRender();
    ~RGBRender();
    virtual int  CheckTextrue(int* width, int* height, SDL_PixelFormatEnum* pixfmt);
    virtual int  FlushTexture(SDL_Texture* pSdlPainter);
private:
    AVPixelFormat m_pfPixfmt;
};

