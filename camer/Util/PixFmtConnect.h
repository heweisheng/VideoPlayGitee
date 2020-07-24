#pragma once
extern "C"
{
#include <libavutil/pixfmt.h>
#include <SDL_pixels.h>
}
typedef struct {
    AVPixelFormat ffmpegfmt;
    SDL_PixelFormatEnum sdlfmt;
}PixList;
bool CheckRGB(SDL_PixelFormatEnum format);

bool CheckYVU(SDL_PixelFormatEnum format);

SDL_PixelFormatEnum FindSDLFmt(AVPixelFormat pixfmt);

AVPixelFormat FindFFmpegFmt(SDL_PixelFormatEnum pixfmt);