#include "pch.h"
#include "SDLManage.h"
extern "C"
{
#include <libavutil/mathematics.h>
#include <libavutil/common.h>
#pragma comment(lib, "avutil.lib")
}

SDLManage::SDLManage()
{
    m_pSdlPainter = NULL;
    m_pSdlRender = NULL;
    m_pSdlScreen = NULL;
    m_pVideoInstance = NULL;
}


SDLManage::~SDLManage()
{
    SDL_Quit();
    if (m_pSdlPainter)
        SDL_DestroyTexture(m_pSdlPainter);
    if (m_pSdlRender)
        SDL_DestroyRenderer(m_pSdlRender);
    if (m_pSdlScreen)
        SDL_DestroyWindow(m_pSdlScreen);
}

int SDLManage::InitVideo_SDL2(const char * userdlgname, int width, int height, SDL_PixelFormatEnum pixformat)
{
    int iRet = 0;
    m_PixFmt = pixformat;
    m_iFrameWidth = width <= 0 ? 1 : width;
    m_iFrameHeight = height <= 0 ? 1 : height;
    m_iScreenWidth = m_iFrameWidth;
    m_iScreenHeight = m_iFrameHeight;
    do 
    {
        SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
        SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
        iRet=SDL_Init(SDL_INIT_VIDEO); 
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        if (iRet)
        {
            break;
        }
        m_pSdlScreen = SDL_CreateWindow(userdlgname, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_iScreenWidth, m_iScreenHeight, SDL_WINDOW_RESIZABLE| SDL_WINDOW_OPENGL);
        if (!m_pSdlScreen)
        {
            iRet = -1;
            break;
        }
        m_pSdlRender = SDL_CreateRenderer(m_pSdlScreen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!m_pSdlRender)
        {
            iRet = -2;
            break;
        }
        m_pSdlPainter = SDL_CreateTexture(m_pSdlRender, pixformat, SDL_TEXTUREACCESS_STREAMING, m_iFrameWidth, m_iFrameHeight);
        if(!m_pSdlPainter)
        {
            iRet = -3;
            break;
        }
    } while (0);
    if (iRet)
    {
        if (m_pSdlPainter)
            SDL_DestroyTexture(m_pSdlPainter);
        if (m_pSdlRender)
            SDL_DestroyRenderer(m_pSdlRender);
        if (m_pSdlScreen)
            SDL_DestroyWindow(m_pSdlScreen);
        m_pSdlPainter = NULL;
        m_pSdlRender = NULL;
        m_pSdlScreen = NULL;
    }
    return iRet;
}

int SDLManage::GetEvent(SDL_Event * event)
{
    return SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
}

void SDLManage::ReSetSize()
{
    SDL_GetWindowSize(m_pSdlScreen, &m_iScreenWidth, &m_iScreenHeight);
}

void SDLManage::CalculateRect(SDL_Rect * rect, int width, int height, int pic_width, int pic_height)
{
    int cal_width, cal_height,x,y;
    cal_height = height;
    cal_width = av_rescale(cal_height,pic_width,pic_height);
    if (cal_width > width)
    {
        cal_width = width;
        cal_height = av_rescale(cal_width, pic_height, pic_width);
    }
    x = (width - cal_width) / 2;
    y = (height - cal_height) / 2;
    rect->x = x;
    rect->y = y;
    rect->w = FFMAX(cal_width, 1);
    rect->h = FFMAX(cal_height, 1);
}

void SDLManage::SetVideoFlush(IVideoFlushSDL2* instance)
{
    m_pVideoInstance = instance;
}

int SDLManage::InitAudio_SDL2(SDL_AudioSpec *audiosapce)
{
    int iRet = 0;
    do {
        if (iRet = SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER))
            break;
        if (iRet = SDL_OpenAudio(audiosapce, NULL))
            break;
        SDL_PauseAudio(0);
    } while (0);
    return iRet;
}

int SDLManage::QuitAudio_SDL2()
{
    SDL_AudioQuit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return 0;
}

int SDLManage::FlushVideoRender()
{
    int iRet = 0;
    SDL_Rect DstRect;
    do {
        if (m_pVideoInstance)
        {
            if (m_pVideoInstance->CheckTextrue(&m_iFrameWidth, &m_iFrameHeight,&m_PixFmt))
            {
                if (m_pSdlPainter)
                    SDL_DestroyTexture(m_pSdlPainter);
                m_pSdlPainter = SDL_CreateTexture(m_pSdlRender, m_PixFmt, SDL_TEXTUREACCESS_STREAMING, m_iFrameWidth, m_iFrameHeight);
                if (!m_pSdlPainter)
                {
                    iRet = -3;
                    break;
                }
            }
            if (iRet=m_pVideoInstance->FlushTexture(m_pSdlPainter))
            {
                break;
            }
        }
        SDL_SetRenderDrawColor(m_pSdlRender, 0xf9, 0xf9, 0xf9, 255);
        if (iRet = SDL_RenderClear(m_pSdlRender))
            break;
        CalculateRect(&DstRect, m_iScreenWidth, m_iScreenHeight, m_iFrameWidth, m_iFrameHeight);
        iRet=SDL_RenderCopy(m_pSdlRender, m_pSdlPainter, NULL, &DstRect);
        if (iRet)
        {
            break;
        }
        SDL_RenderPresent(m_pSdlRender);
    } while (0);
    return iRet;
}
