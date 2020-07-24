#include "pch.h"
#include "RGBRender.h"


RGBRender::RGBRender()
{
    m_pfPixfmt = AV_PIX_FMT_NONE;
}


RGBRender::~RGBRender()
{
}

int RGBRender::CheckTextrue(int * width, int * height, SDL_PixelFormatEnum * pixfmt)
{
    int iRet = 0;
    m_pFrameLock.lock();
    if (m_pFrame&&m_pfPixfmt!=m_pFrame->format)
    {
        m_pfPixfmt = (AVPixelFormat)m_pFrame->format;
        *pixfmt=FindSDLFmt(m_pfPixfmt);
        iRet = 1;
    }
    if (m_pFrame&&(m_pFrame->width != *width || m_pFrame->height != *height))
    {
        *width = m_pFrame->width;
        *height = m_pFrame->height;
        iRet = 1;
    }
    return iRet;
}

int RGBRender::FlushTexture(SDL_Texture * pSdlPainter)
{
    int iRet = 0;
    void *pixels[4];
    int linesize[4];
    if (m_pFrame&&m_bNeedFlush)
    {               
        m_bNeedFlush = false;
        if (m_pFrame)
        {
            iRet = SDL_UpdateTexture(pSdlPainter, NULL, m_pFrame->data[0], m_pFrame->linesize[0]);
        }
            
    }
    m_pFrameLock.unlock();
    return iRet;
}
