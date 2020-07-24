#pragma once
extern "C"
{
#include <SDL.h>
#pragma comment(lib, "SDL2.lib")
}
class IVideoFlushSDL2 {
public:
    /*********************************
    @作用:    检测画板是否有效，需要sdl2重建
    @参数:    width,height画板的长宽参数
    @参数:    pixfmt为像素类型
    @返回值:  1为需要更新，0为不需要
    @备注:
    ***********************************/
    virtual int CheckTextrue(int* width, int* height, SDL_PixelFormatEnum* pixfmt)=0;
    /*********************************
    @作用:    更新画板
    @参数:    pSdlPainter，为需要刷新的画板
    @返回值:  0为成功，其余失败
    @备注:
    ***********************************/
    virtual int FlushTexture(SDL_Texture *pSdlPainter)=0;
};
class SDLManage
{
public:
    SDLManage();
    ~SDLManage();
    /*********************************
    @作用:    SDL2初始化
    @参数:    userdlgname，标题
    @参数:    width，宽度
    @参数:    height,高度
    @参数:    pixformat,画板的像素格式
    @返回值:  0为成功，其余失败
    @备注:
    ***********************************/
    int InitVideo_SDL2(const char* userdlgname,int width,int height, SDL_PixelFormatEnum pixformat);
    /*********************************
    @作用:    获取SDL2的事件，决定是否进行画面刷新
    @参数:    event为事件
    @返回值:  0为没有事件，可以刷新，其余为有事件
    @备注:
    ***********************************/
    int GetEvent(SDL_Event *event);

    /*********************************
    @作用:    根据当前窗口状态，重新设置渲染区域大小
    @参数:
    @备注:
    ***********************************/
    void ReSetSize();

    /*********************************
    @作用:    使渲染画面处于居中位置
    @参数:    width为屏幕宽度，height为屏幕高
    @参数:    pic_width,pic_height为画面宽高
    @返回值:  
    @备注:
    ***********************************/
    void CalculateRect(SDL_Rect* rect, int width, int height, int pic_width, int pic_height);

    /*********************************
    @作用:    设置视频渲染的接口
    @参数:    instance目前已经分出了三个渲染器，yuv跟rgb效率较高
    @备注:
    ***********************************/
    void SetVideoFlush(IVideoFlushSDL2* instance);

    /*********************************
    @作用:    音频初始化
    @参数:    audiosapce为音频数据回调
    @返回值:  
    ***********************************/
    int InitAudio_SDL2(SDL_AudioSpec* audiosapce);
    int QuitAudio_SDL2();
    int FlushVideoRender();
    //SDL的窗口，画板，渲染器句柄
private:
    IVideoFlushSDL2*     m_pVideoInstance;
    SDL_Window*     m_pSdlScreen;
    SDL_Texture*    m_pSdlPainter;
    SDL_Renderer*   m_pSdlRender;
    SDL_PixelFormatEnum m_PixFmt;
    int m_iFrameWidth;
    int m_iFrameHeight;
    int m_iScreenWidth;
    int m_iScreenHeight;
};

