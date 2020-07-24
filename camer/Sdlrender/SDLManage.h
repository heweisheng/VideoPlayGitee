#pragma once
extern "C"
{
#include <SDL.h>
#pragma comment(lib, "SDL2.lib")
}
class IVideoFlushSDL2 {
public:
    /*********************************
    @����:    ��⻭���Ƿ���Ч����Ҫsdl2�ؽ�
    @����:    width,height����ĳ������
    @����:    pixfmtΪ��������
    @����ֵ:  1Ϊ��Ҫ���£�0Ϊ����Ҫ
    @��ע:
    ***********************************/
    virtual int CheckTextrue(int* width, int* height, SDL_PixelFormatEnum* pixfmt)=0;
    /*********************************
    @����:    ���»���
    @����:    pSdlPainter��Ϊ��Ҫˢ�µĻ���
    @����ֵ:  0Ϊ�ɹ�������ʧ��
    @��ע:
    ***********************************/
    virtual int FlushTexture(SDL_Texture *pSdlPainter)=0;
};
class SDLManage
{
public:
    SDLManage();
    ~SDLManage();
    /*********************************
    @����:    SDL2��ʼ��
    @����:    userdlgname������
    @����:    width�����
    @����:    height,�߶�
    @����:    pixformat,��������ظ�ʽ
    @����ֵ:  0Ϊ�ɹ�������ʧ��
    @��ע:
    ***********************************/
    int InitVideo_SDL2(const char* userdlgname,int width,int height, SDL_PixelFormatEnum pixformat);
    /*********************************
    @����:    ��ȡSDL2���¼��������Ƿ���л���ˢ��
    @����:    eventΪ�¼�
    @����ֵ:  0Ϊû���¼�������ˢ�£�����Ϊ���¼�
    @��ע:
    ***********************************/
    int GetEvent(SDL_Event *event);

    /*********************************
    @����:    ���ݵ�ǰ����״̬������������Ⱦ�����С
    @����:
    @��ע:
    ***********************************/
    void ReSetSize();

    /*********************************
    @����:    ʹ��Ⱦ���洦�ھ���λ��
    @����:    widthΪ��Ļ��ȣ�heightΪ��Ļ��
    @����:    pic_width,pic_heightΪ������
    @����ֵ:  
    @��ע:
    ***********************************/
    void CalculateRect(SDL_Rect* rect, int width, int height, int pic_width, int pic_height);

    /*********************************
    @����:    ������Ƶ��Ⱦ�Ľӿ�
    @����:    instanceĿǰ�Ѿ��ֳ���������Ⱦ����yuv��rgbЧ�ʽϸ�
    @��ע:
    ***********************************/
    void SetVideoFlush(IVideoFlushSDL2* instance);

    /*********************************
    @����:    ��Ƶ��ʼ��
    @����:    audiosapceΪ��Ƶ���ݻص�
    @����ֵ:  
    ***********************************/
    int InitAudio_SDL2(SDL_AudioSpec* audiosapce);
    int QuitAudio_SDL2();
    int FlushVideoRender();
    //SDL�Ĵ��ڣ����壬��Ⱦ�����
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

