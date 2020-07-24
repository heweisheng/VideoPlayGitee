#pragma once
#include "SDLManage.h"
#include <mutex>
#include "../Util/PixFmtConnect.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
/*********************************
@����:    �ṩĬ�ϵ���Ⱦ�ӿڣ��ýӿڿ�����Ⱦ����ffmpeg֧�ֵ����ظ�ʽ
@��ע:    �ýӿ��������Դ��
***********************************/
class OtherRender :public IVideoFlushSDL2
{
public:
    OtherRender();
    ~OtherRender();
    /*********************************
    @����:    ֡���£����ǵ����߳�ʹ�ã������̰߳�ȫ
    @����:    frame,����֡
    @��ע:    ���������Ƚϴ󣬲�������check�����������ʲ��ߣ���������ν
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

