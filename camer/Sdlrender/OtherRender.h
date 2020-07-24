#pragma once
#include "SDLManage.h"
#include <mutex>
#include "../Util/PixFmtConnect.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
/*********************************
@作用:    提供默认的渲染接口，该接口可以渲染所有ffmpeg支持的像素格式
@备注:    该接口是最耗资源的
***********************************/
class OtherRender :public IVideoFlushSDL2
{
public:
    OtherRender();
    ~OtherRender();
    /*********************************
    @作用:    帧更新，考虑到多线程使用，已是线程安全
    @参数:    frame,更新帧
    @备注:    现锁的粒度较大，不过考虑check函数触发概率不高，所以无所谓
    ***********************************/
    virtual void SendFrameToCache(AVFrame* frame);
    virtual int  CheckTextrue(int* width, int* height, SDL_PixelFormatEnum* pixfmt);
    virtual int  FlushTexture(SDL_Texture* pSdlPainter);
protected:
    bool        m_bNeedFlush;
    AVFrame*    m_pFrame;    
    std::mutex  m_pFrameLock;

private:
    SwsContext* m_pSws;        
    AVPixelFormat m_pfPixfmt;
};

