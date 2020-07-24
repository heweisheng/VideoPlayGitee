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
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
}
class TranslateVideo
{
public:
    TranslateVideo();
    ~TranslateVideo();
    //总体与TranslateAudio一致
    static int DeCodeframe(
        AVCodecContext * input_codec_context IN,
        AVPacket * video_packet IN,
        AVFrame ** frame OUT, 
        int *finsh OUT);
    static int InitSwsContext(
        SwsContext **convert_context OUT,
        int input_width,
        int input_height,
        AVPixelFormat input_format,
        int ouput_width,
        int ouput_height,
        AVPixelFormat ouput_format
        );
    static int InitImageFrame(
        AVFrame **frame OUT,
        int width,
        int height,
        AVPixelFormat pix_fmt
    );
    static int EnCodePacket(
        SwsContext *swsctx IN,
        AVStream *VideoInStream IN,
        AVStream *VideoOutStream IN,
        AVFrame *Inputframe IN,
        AVFrame *Outputframe IN,
        AVCodecContext *output_codec_context IN,
        AVPacket *video_packet OUT,
        int *finsh OUT,
        int *getpacket OUT
    );
    static int init_input_frame(AVFrame ** frame);
    static int FlushPacket(
        AVStream *VideoOutStream IN,
        AVFormatContext *OutputFmt IN,
        AVCodecContext *VideoEnCodeCtx IN
    );
};

