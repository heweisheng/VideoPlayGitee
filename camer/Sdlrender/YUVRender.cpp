#include "pch.h"
#include "YUVRender.h"
extern "C"
{
#include <libavutil/pixdesc.h>
}

YUVRender::YUVRender()
{
    m_pfPixfmt = AV_PIX_FMT_NONE;
}


YUVRender::~YUVRender()
{
}

int YUVRender::CheckTextrue(int * width, int * height, SDL_PixelFormatEnum * pixfmt)
{
    int iRet = 0;
    m_pFrameLock.lock();
    if (m_pFrame&&m_pfPixfmt != m_pFrame->format)
    {
        m_pfPixfmt = (AVPixelFormat)m_pFrame->format;
        *pixfmt = FindSDLFmt(m_pfPixfmt);
        iRet = 1;
    }
    if (m_pFrame && (m_pFrame->width != *width || m_pFrame->height != *height))
    {
        *width = m_pFrame->width;
        *height = m_pFrame->height;
        iRet = 1;
    }
    return iRet;
}

int YUVRender::FlushTexture(SDL_Texture * pSdlPainter)
{
    int iRet = 0;
    uint8_t *pixels[4];
    int picture[4];
    if (m_pFrame&&m_bNeedFlush)
    {
        //YUV图层不一定是1、2、3分布
        m_bNeedFlush = false;
        if (m_pFrame)
        {
            switch (FindSDLFmt(m_pfPixfmt))
            {            
            case SDL_PIXELFORMAT_IYUV:
            {
                if (m_pFrame->linesize[0] > 0 && m_pFrame->linesize[1] > 0 && m_pFrame->linesize[2] > 0) {
                    iRet = SDL_UpdateYUVTexture(pSdlPainter, NULL, m_pFrame->data[0], m_pFrame->linesize[0],
                        m_pFrame->data[1], m_pFrame->linesize[1],
                        m_pFrame->data[2], m_pFrame->linesize[2]);
                }
                else if (m_pFrame->linesize[0] < 0 && m_pFrame->linesize[1] < 0 && m_pFrame->linesize[2] < 0) {
                    iRet = SDL_UpdateYUVTexture(pSdlPainter, NULL, m_pFrame->data[0] + m_pFrame->linesize[0] * (m_pFrame->height - 1), -m_pFrame->linesize[0],
                        m_pFrame->data[1] + m_pFrame->linesize[1] * (AV_CEIL_RSHIFT(m_pFrame->height, 1) - 1), -m_pFrame->linesize[1],
                        m_pFrame->data[2] + m_pFrame->linesize[2] * (AV_CEIL_RSHIFT(m_pFrame->height, 1) - 1), -m_pFrame->linesize[2]);
                }
                break;
            }
            default:
                if (m_pFrame->linesize[0] < 0) {
                    iRet = SDL_UpdateTexture(pSdlPainter, NULL, m_pFrame->data[0] + m_pFrame->linesize[0] * (m_pFrame->height - 1), -m_pFrame->linesize[0]);
                }
                else {
                    iRet = SDL_UpdateTexture(pSdlPainter, NULL, m_pFrame->data[0], m_pFrame->linesize[0]);
                }
                break;
            }
        }
    }
    m_pFrameLock.unlock();
    return iRet;
}
