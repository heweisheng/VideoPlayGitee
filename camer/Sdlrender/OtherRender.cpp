#include "pch.h"
#include "OtherRender.h"


OtherRender::OtherRender()
{
    m_pFrame = NULL;
    m_pfPixfmt = AV_PIX_FMT_NONE;
    m_pSws = NULL;
    m_bNeedFlush = false;
}


OtherRender::~OtherRender()
{
    //��Ⱦ�̴߳���ͬһ���̣߳�Ͷ�ݿ��Բ���һ���̣߳�ͨ�����ø�ֵ����Ͷ������
    m_bNeedFlush = true;
    m_pFrameLock.lock();
    if (m_pFrame)
        av_frame_free(&m_pFrame);
    m_pFrameLock.unlock();
}

void OtherRender::SendFrameToCache(AVFrame* frame)
{
    if (m_bNeedFlush)
        return;
    m_pFrameLock.lock();   
    //��Ҫˢ�²�����֡����
    if (!m_bNeedFlush)
    {
        if (m_pFrame)
            av_frame_free(&m_pFrame);
        m_pFrame = av_frame_clone(frame);
        m_bNeedFlush = true;
    }
    m_pFrameLock.unlock();
}

int OtherRender::CheckTextrue(int * width, int * height, SDL_PixelFormatEnum * pixfmt)
{
    int iRet = 0;
    //�������ȱȽϴ󣬵���ʵ����ν����Ϊ��Ƶ�����м�䶯���ʺܵͣ������ϲ���Ҫ�ؽ�����
    m_pFrameLock.lock();
    if (m_pFrame&&m_pfPixfmt != m_pFrame->format)
    {
        iRet = 1;
        m_pfPixfmt =(AVPixelFormat)m_pFrame->format;

        *pixfmt=FindSDLFmt(AV_PIX_FMT_BGRA);
        if (m_pSws)
        {
            sws_freeContext(m_pSws);
        }
        m_pSws = sws_getContext(
            m_pFrame->width,
            m_pFrame->height,
            m_pfPixfmt,
            m_pFrame->width,
            m_pFrame->height,
            AV_PIX_FMT_BGRA,
            SWS_BICUBIC,
            NULL,
            NULL,
            NULL
        );
    }
    if (m_pFrame&&(m_pFrame->width!=*width||m_pFrame->height!=*height))
    {
        iRet = 1;
        *width = m_pFrame->width;
        *height = m_pFrame->height;
    }
    return iRet;
}

int OtherRender::FlushTexture(SDL_Texture * pSdlPainter)
{
    uint8_t*pixels[4];
    int   picture[4];
    int iRet = 0;
    do {
        if (m_bNeedFlush&&m_pFrame)
        {
            //rgbaû��planer�����Կ�����������
            if (iRet=SDL_LockTexture(pSdlPainter, NULL, (void **)pixels, picture))
                break;
            m_bNeedFlush = false;
            if (m_pFrame)
                sws_scale(m_pSws, m_pFrame->data, m_pFrame->linesize, 0,
                    m_pFrame->height, pixels, picture
                );
            SDL_UnlockTexture(pSdlPainter);
        }        
    } while (0);
    m_pFrameLock.unlock();
    return iRet;
}
