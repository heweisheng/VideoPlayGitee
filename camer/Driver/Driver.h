#pragma once
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavdevice/avdevice.h>
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avutil.lib")
}
class Driver
{
public:
    Driver();
    ~Driver();
    int Init_camerdriver(const char* dev);
    int Init_screendriver();
    int GetFrameFromCamer(AVFrame **frame);
    AVFormatContext* m_pCamer;
    AVCodecContext* m_pDeCoder;
    static int Frame_2Picture(const char *name, AVFrame *pFrame, AVCodecID EnCodeID = AV_CODEC_ID_NONE);
    static int InitImageFrame(
        AVFrame **frame,
        int width,
        int height,
        AVPixelFormat pix_fmt
    );
};

