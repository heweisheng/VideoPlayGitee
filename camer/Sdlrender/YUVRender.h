#pragma once
#include "OtherRender.h"
class YUVRender: public OtherRender
{
public:
    YUVRender();
    ~YUVRender();
    virtual int CheckTextrue(int* width, int* height, SDL_PixelFormatEnum* pixfmt);
    //ˢ�»��壬0Ϊ�ɹ�������ʧ��
    virtual int FlushTexture(SDL_Texture *pSdlPainter);
private:
    AVPixelFormat m_pfPixfmt;
};

