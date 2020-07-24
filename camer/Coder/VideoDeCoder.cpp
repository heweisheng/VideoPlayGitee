#include "pch.h"
#include "VideoDeCoder.h"
#include "TranslateVideo.h"
#include "TranslateAudio.h"
#include "SaveToFile.h"
SaveToFile Handle;
#define CODECTX_FREE(m)\
do {\
    if(m)\
        avcodec_free_context(&m);\
} while(0)
#define PI 3.141592653589793
extern char g_sError_ptr[AV_ERROR_MAX_STRING_SIZE];
extern char g_sError_where[1024];
VideoDeCoder::VideoDeCoder()
{
	m_bCoding = false;
    m_pAudioDeCoder = NULL;
    m_pAudioStream = NULL;
    m_pVideoDeCoder = NULL;
    m_pVideoStream = NULL;
    m_pInputFormat = NULL;
    m_bend = false;
    m_hHandle = NULL;
    m_bDoing = false;
    m_pSwr = NULL;
    m_pFifo = NULL;
    m_pReSampleCache = NULL;
}


VideoDeCoder::~VideoDeCoder()
{
	if (m_hHandle)
		ThreadPoolH::GetInstance()->Join(m_hHandle);
	while (!ThreadPoolH::GetInstance()->Release());
	if (m_bCoding)
	{
		Handle.Save(false, NULL, m_pVideoDeCoder, m_sfilename.c_str());
	}
    if (m_pAudioDeCoder)
        avcodec_free_context(&m_pAudioDeCoder);
    if (m_pVideoDeCoder)
        avcodec_free_context(&m_pVideoDeCoder);
    if (m_pInputFormat)
    {
        //if(m_pInputFormat->pb)
        //    avio_close(m_pInputFormat->pb);
        avformat_close_input(&m_pInputFormat);
    }
    if (m_pSwr)
    {
        swr_free(&m_pSwr);
    }
    if (m_pFifo)
    {
        av_audio_fifo_free(m_pFifo);
    }
    if (m_pReSampleCache)
    {
        av_freep(&m_pReSampleCache[0]);
        free(m_pReSampleCache);
    }
}

int VideoDeCoder::InitDeCoder(const char * filename)
{
    int videostream = -1;
    int audiostream = -1;
    int iRet = InitInputContext(filename, &m_pInputFormat, &m_pAudioDeCoder, &m_pVideoDeCoder, &audiostream, &videostream);
    if (videostream != -1)
        m_pVideoStream = m_pInputFormat->streams[videostream];
    if (audiostream != -1)
        m_pAudioStream = m_pInputFormat->streams[audiostream];
    return iRet;
}

int VideoDeCoder::InitInputContext(const char * filename, AVFormatContext **InputFormatCtx OUT, AVCodecContext **AudioCodeCtx OUT, AVCodecContext **VideoCodeCtx OUT, int *audioinput OUT, int *videoinput OUT)
{
    AVFormatContext *pFmtCtx = NULL;
    AVCodecContext *AudioCtx = NULL;
    AVCodecContext *VideoCtx = NULL;
    AVCodec *Audiocodec = NULL;
    AVCodec *Videocodec = NULL;
    int iRet = 0;
    do {
        iRet = avformat_open_input(&pFmtCtx, filename, NULL, NULL);
        if (iRet < 0)
        {
            printf("create input error \n%s %d\n", __FILE__, __LINE__);
            break;
        }
        iRet = avformat_find_stream_info(pFmtCtx, NULL);
        if (iRet < 0)
        {
            printf("without input stream in the video \n%s %d\n", __FILE__, __LINE__);
            break;
        }
        for (size_t i = 0; i < pFmtCtx->nb_streams; i++)
        {
            //目前没有支持多流的设计方案，经验不足先支持单流
            if (pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                *videoinput = i;
                Videocodec = avcodec_find_decoder(pFmtCtx->streams[i]->codecpar->codec_id);
                if (!Videocodec)
                {
                    iRet = -1;
                    printf("can't create vcodec \n%s %d\n", __FILE__, __LINE__);
                    break;
                }
            }
            else if (pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                *audioinput = i;
                Audiocodec = avcodec_find_decoder(pFmtCtx->streams[i]->codecpar->codec_id);
                if (!Audiocodec)
                {
                    iRet = -1;
                    printf("can't create acodec \n%s %d\n", __FILE__, __LINE__);
                    break;
                }
            }

        }
        if (iRet < 0)
            break;

        if (Audiocodec) {
            AudioCtx = avcodec_alloc_context3(Audiocodec);
            if (!AudioCtx)
            {
                iRet = -1;
                printf("can't create audioctx \n%s %d\n", __FILE__, __LINE__);
                break;
            }
            iRet = avcodec_parameters_to_context(AudioCtx, pFmtCtx->streams[*audioinput]->codecpar);
            if (iRet < 0)
            {
                printf("copy context error\n%s %d\n", __FILE__, __LINE__);
                break;
            }
            if ((iRet = avcodec_open2(AudioCtx, Audiocodec, NULL)) < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder\n %s %s", __FILE__, __LINE__);
                break;
            }
        }
        if (Videocodec)
        {
            VideoCtx = avcodec_alloc_context3(Videocodec);
            if (!VideoCtx)
            {
                iRet = -1;
                printf("can't create videoctx\n%s %d\n", __FILE__, __LINE__);
                break;
            }
            iRet = avcodec_parameters_to_context(VideoCtx, pFmtCtx->streams[*videoinput]->codecpar);
            if (iRet < 0)
            {
                printf("copy context error\n%s %d\n", __FILE__, __LINE__);
                break;
            }
            //VideoCtx->time_base = pFmtCtx->streams[videoinput]->codec->time_base;
            VideoCtx->framerate = av_guess_frame_rate(pFmtCtx, pFmtCtx->streams[*videoinput], NULL);
            if ((iRet = avcodec_open2(VideoCtx, Videocodec, NULL)) < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n%s %d\n", __FILE__, __LINE__);
                break;
            }
        }
    } while (0);
    if (iRet)
    {
        if (pFmtCtx)
            avformat_close_input(&pFmtCtx);
        CODECTX_FREE(AudioCtx);
        CODECTX_FREE(VideoCtx);
        return iRet;
    }
    *InputFormatCtx = pFmtCtx;
    if (AudioCodeCtx)
        *AudioCodeCtx = AudioCtx;
    else {
        CODECTX_FREE(AudioCtx);
    }
    if (VideoCodeCtx)
        *VideoCodeCtx = VideoCtx;
    else
    {
        CODECTX_FREE(VideoCtx);
    }
    return 0;
}

AVFrame * VideoDeCoder::GetVideoFrame()
{
    AVFrame* retframe = NULL;
    m_pVideoMutex.lock();
    if (!m_vVideoCache.empty())
    {
        retframe = m_vVideoCache.front();
        m_vVideoCache.pop();
    }
    m_pVideoMutex.unlock();
    StartCache();
    return retframe;
}

void VideoDeCoder::Coding(const char* filename)
{
	m_bCoding = true;
	m_sfilename = filename;
}

void VideoDeCoder::EndCoding()
{
	m_bCoding = false;
}

AVAudioFifo * VideoDeCoder::GetAudioFifo()
{    
    if (!m_pVideoDeCoder)
    {
        m_pAudioMutex.lock();
        if (av_audio_fifo_size(m_pFifo) < 10240)
        {
            m_pAudioMutex.unlock();
            StartCache();
        }
        else
            m_pAudioMutex.unlock();
    }
    return m_pFifo;
}


void VideoDeCoder::StartCache()
{    
    if (!m_bDoing)
    {
        m_pMissionMutex.lock();
        if (!m_bDoing&&((m_pVideoDeCoder&&m_vVideoCache.size() < VIDEO_MAXFRAME / 2) || (!m_pVideoDeCoder&&av_audio_fifo_size(m_pFifo) < 10240)))
        {
            m_bDoing = true;
            ThreadPoolH::GetInstance()->PushMission(this, false);
        }
        m_pMissionMutex.unlock();
    }
    return;
}

int VideoDeCoder::InitAudioSwrsample(AVSampleFormat fmt, int channels)
{
    int error;
    SwrContext* pResample_context;
    /*
     * Create a resampler context for the conversion.
     * Set the conversion parameters.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity (they are sometimes not detected
     * properly by the demuxer and/or m_pthreadencoder).
     */
    pResample_context = swr_alloc_set_opts(NULL,
        av_get_default_channel_layout(channels),
        fmt,
        m_pAudioDeCoder->sample_rate,
        av_get_default_channel_layout(m_pAudioDeCoder->channels),
        m_pAudioDeCoder->sample_fmt,
        m_pAudioDeCoder->sample_rate,
        0, NULL);
    if (!pResample_context) {
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        sprintf_s(g_sError_where, "Could not allocate resample context\n%s %d\n", __FILE__, __LINE__);
        return AVERROR(ENOMEM);
    }
    /*
    * Perform a sanity check so that the number of converted samples is
    * not greater than the number of samples to be converted.
    * If the sample rates differ, this case has to be handled differently
    */
    //这里被修改了,可能会出问题
    //av_assert0(pAudioContext->sample_rate == input_codec_context->sample_rate);

    /* Open the resampler with the specified parameters. */
    if ((error = swr_init(pResample_context)) < 0) {
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, error);
        sprintf_s(g_sError_where, "Could not open resample context\n%s %d\n", __FILE__, __LINE__);
        swr_free(&pResample_context);
        return error;
    }
    //TranslateAudio::Init_converted_samples(&m_pReSampleCache,)
    int iRet = 0;
    do {
        iRet = InitAudioSwrCache(&m_pReSampleCache, fmt, channels, 100000);
        if (iRet)
            break;
        iRet = InitAudioSwrFifo(&m_pFifo, fmt, channels);
        if (iRet)
            break;
        m_pSwr = pResample_context;
    }while (0);
    return 0;
}

int VideoDeCoder::InitAudioSwrCache(uint8_t *** Cache, AVSampleFormat fmt, int channels, int nb_sample)
{
    int error;

    /* Allocate as many pointers as there are audio channels.
     * Each pointer will later point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     */
    if (!(*Cache = (uint8_t **)calloc(channels,
        sizeof(**Cache)))) {
        sprintf_s(g_sError_where, "Could not allocate converted input sample pointers\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        *Cache = NULL;
        return AVERROR(ENOMEM);
    }

    /* Allocate memory for the samples of all channels in one consecutive
     * block for convenience. */
    if ((error = av_samples_alloc(*Cache, NULL,
        channels,
        nb_sample,
        fmt, 0)) < 0) {
        sprintf_s(g_sError_where, "Could not allocate converted input samples\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, error);
        av_freep(&(*Cache)[0]);
        free(*Cache);
        *Cache = NULL;
        return error;
    }
    return 0;
}

int VideoDeCoder::InitAudioSwrFifo(AVAudioFifo ** fifo, AVSampleFormat fmt,int channels)
{
    if (!(*fifo = av_audio_fifo_alloc(fmt,
        channels, 1))) {
        sprintf_s(g_sError_where, "Could not allocate FIFO\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        return AVERROR(ENOMEM);
    }
    return 0;
}



void VideoDeCoder::RunFunction()
{
    AVPacket packet;
    AVFrame *pFrame;
    av_init_packet(&packet);
    int finish = false;
    int iRet = 0;
    do {
        while (!iRet && !m_bend)
        {
            if (m_pInputFormat)
            {
                iRet = av_read_frame(m_pInputFormat, &packet);
                if (iRet)
                {
                    m_bend = true;
                    break;
                }
                if (m_pVideoStream&&packet.stream_index == m_pVideoStream->index)
                {
					Handle.Save(m_bCoding, &packet, m_pVideoDeCoder, m_sfilename.c_str());
                    iRet = TranslateVideo::DeCodeframe(m_pVideoDeCoder, &packet, &pFrame, &finish);
                    if (finish)
                        m_bend = true;
                    m_pVideoMutex.lock();
                    if (pFrame)
                        m_vVideoCache.push(pFrame);
                    if (m_vVideoCache.size() >= VIDEO_MAXFRAME)
                        iRet = 1;
                    m_pVideoMutex.unlock();
                }
                else if (m_pAudioStream&&packet.stream_index == m_pAudioStream->index)
                {
                    iRet = TranslateAudio::DeCodeframe(m_pAudioDeCoder, &packet, &pFrame, &finish);
                    if (finish)
                        m_bend = true;
                    m_pAudioMutex.lock();
                    if (pFrame)
                    {
                        TranslateAudio::Convert_samples((const uint8_t**)pFrame->extended_data, m_pReSampleCache, pFrame->nb_samples, pFrame->nb_samples, m_pSwr);
                        TranslateAudio::Add_samples_to_fifo(m_pFifo, m_pReSampleCache, pFrame->nb_samples);
                        av_frame_free(&pFrame);
                    }
                    if (av_audio_fifo_size(m_pFifo) > 20480)
                        iRet = 1;
                    m_pAudioMutex.unlock();
                }
                //av_packet_unref(&packet);
            }
        }
    } while (0);
    m_pMissionMutex.lock();
    m_bDoing = false;
    m_pMissionMutex.unlock();
}
