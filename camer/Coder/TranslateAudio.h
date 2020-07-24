#pragma once
#include <stdio.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#include <libavcodec/avcodec.h>

#include <libavutil/audio_fifo.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>

#include <libswresample/swresample.h>
#pragma comment(lib, "swresample.lib")
}

class TranslateAudio
{
public:
    TranslateAudio();
    ~TranslateAudio();
    //该模块来自ffmpeg的官方demo大写开头字母是我需要用到的部分，DeCode跟EnCode是自己总结写的，可能存在bug
    //官方的aac转换
    //解码成帧
    static int DeCodeframe(
        AVCodecContext *input_codec_context IN,
        AVPacket       *audio_packet        IN,
        AVFrame        **frame              OUT, 
        int            *finsh               OUT);
    //刷出编码器的数据
    static int FlushPacket(
        AVStream       *audio_stream        IN, 
        AVFormatContext*output_fmt_context  IN, 
        AVCodecContext *output_codec_context OUT, 
        AVPacket       *audio_packet        IN
    );
    //初始化音频帧
    static int Init_input_frame(AVFrame **frame IN);
    //初始化音频重采样容器
    static int Init_resampler(
        AVCodecContext *input_codec_context IN,
        AVCodecContext *output_codec_context IN,
        SwrContext **resample_context OUT);
    //初始化管道
    static int Init_fifo(
        AVAudioFifo **fifo , 
        AVCodecContext *output_codec_context);
    //编码器编成packet
    static int EnCodePacket(
        AVAudioFifo *fifo IN,
        AVStream *audio_stream IN,
        AVCodecContext *output_codec_context IN,
        AVPacket *Packet OUT,
        int64_t *Pts IN OUT,
        int *getpacket OUT
    );
    //初始化重采样存储空间
    static int Init_converted_samples(
        uint8_t ***converted_input_samples OUT,
        AVCodecContext *output_codec_context IN,
        int frame_size);

    //下面部分看函数名就知道了
    static int Convert_samples(const uint8_t **input_data,
        uint8_t **converted_data, const int input_size,const int output_size,
        SwrContext *resample_context);
    static int Add_samples_to_fifo(AVAudioFifo *fifo,
        uint8_t **converted_input_samples,
        const int frame_size);
    static int InsertEmptyVoice(
        AVFormatContext* pFmtCtxDest, //写入的文件格式上下文
        AVCodecContext *AudioCodeCtx,
        AVStream*        pStreamDest, //写入的输出流
        int64_t          *pPtsStart,   //开始写入的时间戳
        int64_t          iPtsEnd     //结束写入的时间撮
    );
private:
    static int Init_output_frame(AVFrame **frame,
        AVCodecContext *output_codec_context,
        int frame_size);
    static void Init_packet(AVPacket *packet);
};

