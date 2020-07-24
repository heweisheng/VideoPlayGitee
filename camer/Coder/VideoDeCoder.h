#pragma once
#include <vector>
#include <queue>
#include <string>
#include <mutex>
#include <Threadpoolh.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}
#define VIDEO_MAXFRAME 200
#define AUDIO_MAXFRAME 100
class VideoDeCoder : public IThreadMission
{
public:
    VideoDeCoder();
    ~VideoDeCoder();
    int InitDeCoder(const char* filename);
    /*********************************
    @作用:    通过输入的文件进行解码器初始化
    @参数:    filename，文件名
    @参数:    InputFormatCtx，输入流容器
    @参数:    AudioCodeCtx，解码器
    @参数:    VideoCodeCtx,编码器
    @返回值:  0为正确，其余错误
    @备注:
    ***********************************/
    static int InitInputContext(
        const char *filename,
        AVFormatContext **InputFormatCtx   OUT,
        AVCodecContext **AudioCodeCtx      OUT,
        AVCodecContext **VideoCodeCtx      OUT,
        int            *audioinput         OUT,
        int            *videoinput         OUT
    );

    AVFrame *GetVideoFrame();
	void Coding(const char *filename);
	void EndCoding();
    AVAudioFifo *GetAudioFifo();
    void StartCache();
    int InitAudioSwrsample(AVSampleFormat fmt,int channels);
    int InitAudioSwrCache(uint8_t*** Cache,AVSampleFormat fmt,int channels,int nb_sample);
    int InitAudioSwrFifo(AVAudioFifo **fifo,AVSampleFormat fmt, int channels);
    virtual void RunFunction();

    std::queue<AVFrame*>    m_vVideoCache;
    //std::queue<AVFrame*>    m_vAudioCache;
    AVFormatContext*        m_pInputFormat;
    SwrContext*             m_pSwr;
    AVAudioFifo*            m_pFifo;
    uint8_t**               m_pReSampleCache;
    AVStream*               m_pAudioStream;
    AVStream*               m_pVideoStream;
    AVCodecContext*         m_pAudioDeCoder;
    AVCodecContext*         m_pVideoDeCoder;
    bool                    m_bDoing;
    HANDLE                  m_hHandle;
    std::mutex              m_pMissionMutex;
    std::mutex              m_pAudioMutex;
	bool					m_bCoding;
    std::mutex              m_pVideoMutex;
    std::atomic_bool        m_bend;
	std::string				m_sfilename;
};

