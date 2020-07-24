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
    //��ģ������ffmpeg�Ĺٷ�demo��д��ͷ��ĸ������Ҫ�õ��Ĳ��֣�DeCode��EnCode���Լ��ܽ�д�ģ����ܴ���bug
    //�ٷ���aacת��
    //�����֡
    static int DeCodeframe(
        AVCodecContext *input_codec_context IN,
        AVPacket       *audio_packet        IN,
        AVFrame        **frame              OUT, 
        int            *finsh               OUT);
    //ˢ��������������
    static int FlushPacket(
        AVStream       *audio_stream        IN, 
        AVFormatContext*output_fmt_context  IN, 
        AVCodecContext *output_codec_context OUT, 
        AVPacket       *audio_packet        IN
    );
    //��ʼ����Ƶ֡
    static int Init_input_frame(AVFrame **frame IN);
    //��ʼ����Ƶ�ز�������
    static int Init_resampler(
        AVCodecContext *input_codec_context IN,
        AVCodecContext *output_codec_context IN,
        SwrContext **resample_context OUT);
    //��ʼ���ܵ�
    static int Init_fifo(
        AVAudioFifo **fifo , 
        AVCodecContext *output_codec_context);
    //���������packet
    static int EnCodePacket(
        AVAudioFifo *fifo IN,
        AVStream *audio_stream IN,
        AVCodecContext *output_codec_context IN,
        AVPacket *Packet OUT,
        int64_t *Pts IN OUT,
        int *getpacket OUT
    );
    //��ʼ���ز����洢�ռ�
    static int Init_converted_samples(
        uint8_t ***converted_input_samples OUT,
        AVCodecContext *output_codec_context IN,
        int frame_size);

    //���沿�ֿ���������֪����
    static int Convert_samples(const uint8_t **input_data,
        uint8_t **converted_data, const int input_size,const int output_size,
        SwrContext *resample_context);
    static int Add_samples_to_fifo(AVAudioFifo *fifo,
        uint8_t **converted_input_samples,
        const int frame_size);
    static int InsertEmptyVoice(
        AVFormatContext* pFmtCtxDest, //д����ļ���ʽ������
        AVCodecContext *AudioCodeCtx,
        AVStream*        pStreamDest, //д��������
        int64_t          *pPtsStart,   //��ʼд���ʱ���
        int64_t          iPtsEnd     //����д���ʱ���
    );
private:
    static int Init_output_frame(AVFrame **frame,
        AVCodecContext *output_codec_context,
        int frame_size);
    static void Init_packet(AVPacket *packet);
};

