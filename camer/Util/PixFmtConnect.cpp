#include "pch.h"
#include "PixFmtConnect.h"
const PixList fmt_list[] = {
    { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
    { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
    { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
    { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
    { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
    { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
    { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
    { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
    { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
    { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
    { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
    { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
    { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
    { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
    { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
    { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
    { AV_PIX_FMT_BGRA,           SDL_PIXELFORMAT_BGRA8888 },

    { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
    { AV_PIX_FMT_YUVJ420P,       SDL_PIXELFORMAT_IYUV },
    { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
    { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
    { AV_PIX_FMT_YUV444P,        SDL_PIXELFORMAT_IYUV},
    { AV_PIX_FMT_YUVJ444P,       SDL_PIXELFORMAT_IYUV},
    { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN }
};

bool CheckRGB(SDL_PixelFormatEnum format)
{
    bool bRet = false;
    const SDL_PixelFormatEnum rgblist[] = {
        SDL_PIXELFORMAT_RGB332 ,
        SDL_PIXELFORMAT_RGB444 ,
        SDL_PIXELFORMAT_RGB555 ,
        SDL_PIXELFORMAT_RGB565 ,
        SDL_PIXELFORMAT_RGB24  ,
        SDL_PIXELFORMAT_RGB888 ,
        SDL_PIXELFORMAT_RGBX8888,
        SDL_PIXELFORMAT_BGRX8888,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_PIXELFORMAT_BGRA8888,
        SDL_PIXELFORMAT_UNKNOWN
    };
    for (int i = 0; rgblist[i] != SDL_PIXELFORMAT_UNKNOWN; i++)
    {
        if (rgblist[i] == format)
        {
            bRet = true;
            break;
        }
    }
    return bRet;
}

bool CheckYVU(SDL_PixelFormatEnum format) {
    bool bRet = false;
    const SDL_PixelFormatEnum rgblist[] = {
        SDL_PIXELFORMAT_IYUV ,
        SDL_PIXELFORMAT_YUY2 ,
        SDL_PIXELFORMAT_UYVY ,
        SDL_PIXELFORMAT_UNKNOWN ,
    };
    for (int i = 0; rgblist[i] != SDL_PIXELFORMAT_UNKNOWN; i++)
    {
        if (rgblist[i] == format)
        {
            bRet = true;
            break;
        }
    }
    return bRet;
}

SDL_PixelFormatEnum FindSDLFmt(AVPixelFormat pixfmt)
{
    SDL_PixelFormatEnum Ret = SDL_PIXELFORMAT_UNKNOWN;
    for (int i = 0; fmt_list[i].ffmpegfmt != AV_PIX_FMT_NONE; i++)
    {
        if (fmt_list[i].ffmpegfmt == pixfmt)
        {
            return fmt_list[i].sdlfmt;
        }
    }
    return Ret;
}

AVPixelFormat FindFFmpegFmt(SDL_PixelFormatEnum pixfmt)
{
    AVPixelFormat Ret = AV_PIX_FMT_NONE;
    for (int i = 0; fmt_list[i].sdlfmt != SDL_PIXELFORMAT_UNKNOWN; i++)
    {
        if (fmt_list[i].sdlfmt == pixfmt)
            return fmt_list[i].ffmpegfmt;
    }
    return Ret;
}
