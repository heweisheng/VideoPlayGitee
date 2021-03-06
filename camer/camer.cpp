// camer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "Driver/Driver.h"
#include "VideoDeCoder.h"
#include "Sdlrender/RGBRender.h"
#include "Sdlrender/YUVRender.h"
#include "TranslateAudio.h"
#include "TranslateVideo.h"
#include "windows.h"
extern "C"
{
#include <SDL.h>
#include <SDL_thread.h>
#include <libavutil/time.h>
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")
}
#ifdef _WIN32
#undef main /* We don't want SDL to override our main() */
#endif

OtherRender* SetRender(AVPixelFormat fmt)
{
	OtherRender *pRender = NULL;
	if (CheckRGB(FindSDLFmt(fmt)))
		pRender = new RGBRender();
	else if (CheckYVU(FindSDLFmt(fmt)))
		pRender = new YUVRender();
	else
		pRender = new OtherRender();
	return pRender;
}
std::atomic_int64_t audiotime = 0;
std::atomic_int64_t videotime = 0;
void SDLCALL SDL_AudioPlay(void* cls, unsigned char* stream, int len)
{
	VideoDeCoder *test = (VideoDeCoder *)cls;
	int nb_sample = len / 2 / test->m_pAudioDeCoder->channels;
	if (test)
	{
		do {
			AVAudioFifo* pFifo = test->GetAudioFifo();
			uint8_t *audio = new uint8_t[len];
			if (!pFifo || (audiotime > videotime))
			{
				memset(stream, 0, len);
				break;
			}
			test->m_pAudioMutex.lock();
			int fifonb_sample = av_audio_fifo_size(pFifo);
			if (fifonb_sample >= nb_sample)
			{
				av_audio_fifo_read(pFifo, (void **)&audio, nb_sample);
				audiotime += av_rescale_q(nb_sample, test->m_pAudioDeCoder->time_base, { 1,AV_TIME_BASE });
				memcpy(stream, audio, len);
			}
			else
			{
				audiotime += av_rescale_q(fifonb_sample, test->m_pAudioDeCoder->time_base, { 1,AV_TIME_BASE });
				int fifolen = fifonb_sample * 2 * test->m_pAudioDeCoder->channels;
				av_audio_fifo_read(pFifo, (void **)&audio, fifonb_sample);
				memcpy(stream, audio, fifolen);
				memset(stream + fifolen, 0, len - fifolen);
			}
			test->m_pAudioMutex.unlock();
			free(audio);
		} while (0);
	}
}
#if PLAYER
/*********************************
@参数: f <file>   播放文件
@参数: s          桌面
@参数: c <drive>  摄像头
@返回值:
@备注: 文件部分没有做高质量的音画同步,高质量的音画同步需要精细设计 目前没有配置release版本
***********************************/
int main(int argc, char** argv)
{
	Driver          CamerDriver;
	VideoDeCoder    VideoDeCoder;
	int             iRet = 0;
	AVFrame*        pFrame = NULL;
	OtherRender*    pRender = NULL;
	SDLManage       sdlmanage;
	bool            bFlush = false;
	AVPixelFormat   fmt;
	bool            quit = false;
	SDL_Event       event;
	int64_t         starttime = 0;
	int64_t         nowtime = 0;
	int64_t         lasttime = 0;
	int             state = 0;
	int64_t         ilastpts = 0;
	bool			bCode = false;
	int				iFrame = 0;//帧计数器，用来计算编码速度
	const char*     testurl = "D://test.mp4";
	do
	{
		if (argc < 3 || (argc == 2 && argv[1][0] != 's'))
		{
			printf("pleass input video file to cmd");
			iRet = -1;
			break;
		}
		if (argv[1][0] == 'c')
		{
			if (iRet = CamerDriver.Init_camerdriver(/*"hm1091_techfront"*/argv[2]))
				break;
			fmt = CamerDriver.m_pDeCoder->pix_fmt;
			state = 1;
		}
		else if (argv[1][0] == 's')
		{
			if (iRet = CamerDriver.Init_screendriver())
				break;
			fmt = CamerDriver.m_pDeCoder->pix_fmt;
			state = 2;
		}
		else if (argv[1][0] == 'f')
		{
			if (iRet = VideoDeCoder.InitDeCoder(argv[2]))
				break;
			fmt = VideoDeCoder.m_pVideoDeCoder->pix_fmt;
			if (VideoDeCoder.m_pAudioStream)
				audiotime = av_rescale_q(VideoDeCoder.m_pAudioStream->start_time, VideoDeCoder.m_pAudioStream->time_base, { 1,AV_TIME_BASE });
			state = 3;
		}
		else
			break;
		pRender = SetRender(fmt);
		sdlmanage.SetVideoFlush(pRender);
		if (state == 1)
		{
			if (iRet = sdlmanage.InitVideo_SDL2("Heweisheng",
				CamerDriver.m_pDeCoder->width,
				CamerDriver.m_pDeCoder->height,
				FindSDLFmt(fmt) == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_RGBA8888 : FindSDLFmt(fmt))
				)
				break;
		}
		else
		{
			if (iRet = sdlmanage.InitVideo_SDL2("Heweisheng",
				1280,
				720,
				FindSDLFmt(fmt) == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_RGBA8888 : FindSDLFmt(fmt))
				)
				break;
		}
		if (state == 3)
		{
			if (VideoDeCoder.m_pAudioDeCoder)
			{
				VideoDeCoder.InitAudioSwrsample(AV_SAMPLE_FMT_S16, VideoDeCoder.m_pAudioDeCoder->channels);
				SDL_AudioSpec spec;
				memset(&spec, 0, sizeof(spec));
				spec.callback = &SDL_AudioPlay;
				spec.channels = VideoDeCoder.m_pAudioDeCoder->channels;
				spec.format = AUDIO_S16SYS;
				spec.freq = VideoDeCoder.m_pAudioDeCoder->sample_rate;
				spec.silence = 0;
				spec.samples = VideoDeCoder.m_pAudioDeCoder->frame_size > 0 ? VideoDeCoder.m_pAudioDeCoder->frame_size : 1024;
				spec.userdata = (void *)&VideoDeCoder;
				sdlmanage.InitAudio_SDL2(&spec);
			}
		}
		starttime = av_gettime();
		while (!quit && !iRet)
		{
			SDL_PumpEvents();
			if (!sdlmanage.GetEvent(&event))
			{
				bFlush = true;
			}
			else
			{
				switch (event.type)
				{
				case SDL_WINDOWEVENT:
				{
					switch (event.window.event)
					{
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						sdlmanage.ReSetSize();
						bFlush = true;
						break;
					case SDL_WINDOWEVENT_EXPOSED:
						bFlush = true;
						break;
					case SDL_WINDOWEVENT_CLOSE:
						quit = true;
						break;
					}
					break;
				}
				case SDL_KEYDOWN:
				{
					if (event.key.keysym.sym == SDLK_e)
					{
						if (!bCode)
						{
							bCode = true;
							VideoDeCoder.Coding(testurl);
						}
						else
						{
							bCode = false;
							VideoDeCoder.EndCoding();
							std::string url = "save sucessful:";
							url += testurl;
							MessageBoxA(NULL, testurl, "tip", NULL);
						}
					}
					break;
				}
				default:
					break;
				}
			}
			if (bFlush)
			{				
				if (pFrame)
				{
					iFrame++;
					if (!(iFrame % 30))
					{
						printf("处理30帧，耗时 %lld ms\n", av_gettime() - starttime);
					}
					ilastpts = pFrame->pts;
					av_frame_free(&pFrame);
				}
				if (state == 1 || state == 2)
				{

					CamerDriver.GetFrameFromCamer(&pFrame);
					pFrame->pts = av_gettime() - starttime;
					if (!ilastpts)
					{
						ilastpts = pFrame->pts;
					}
				}
				if (state == 3)
				{
					pFrame = VideoDeCoder.GetVideoFrame();
					if (pFrame)
					{
						pFrame->pts = av_rescale_q(pFrame->pts, VideoDeCoder.m_pVideoStream->time_base, { 1,AV_TIME_BASE });
						videotime = pFrame->pts;
					}
				}



				if (pFrame&&pFrame->format != fmt)
				{
					fmt = (AVPixelFormat)pFrame->format;
					if (pRender)
						delete pRender;
					pRender = SetRender(fmt);
					sdlmanage.SetVideoFlush(pRender);
				}
				if (pFrame)
				{
					pRender->SendFrameToCache(pFrame);
				}								
				iRet = sdlmanage.FlushVideoRender();	
				nowtime = av_gettime();
				//睡眠公式 两帧时间差-渲染时间总长(对播放有效，延迟问题不是很明显，但发现对摄像头不是太有效延迟有点大)所以改成固定时间，若想做成播放器，建议对同步方面进行设计，参考ffplay
				if (pFrame&&state != 1 && pFrame->pts != AV_NOPTS_VALUE && (pFrame->pts - ilastpts) - (nowtime - lasttime) > 0LL)
				{
					Sleep((pFrame->pts - ilastpts - (nowtime - lasttime)) / 1000);
				}
				else if (state == 1)
				{
					Sleep(20);
				}			
				lasttime = av_gettime();
			}
		}
	} while (0);
	return 0;
}
#endif
#if COLLECTION
/*********************************
@参数: c <drive>  摄像头
@返回值:
@备注: 用于推流功能测试,目前只设计了摄像头的,时间急忙跟部分涉密没有返回值检测跟错误日志，正常情况可以正常运行（仅封装核心部分）
***********************************/
int main(int argc, char** argv)
{
	Driver          CamerDriver;
	AVFrame*		pFrame=NULL;
	AVFormatContext* pFmtContext=NULL;
	AVCodecContext*	pVideoCodecCtx = NULL;
	AVStream*		m_pVideoStream = NULL;
	AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_H265);
	SwsContext *pSws = NULL;
	AVFrame*		pWriteFrame = NULL;
	AVPacket		packet;
	int finsh=0;
	int vget=0;
	int time = 0;
	av_init_packet(&packet);
	int64_t realtime = 0;
	int64_t lastpts = 0;
	const char* filename = "tcp://127.0.0.1:1234?listen";
	int iRet = 0;
	do {
		if (argv[1][0] == 'c')
		{
			if (iRet = CamerDriver.Init_camerdriver(/*"hm1091_techfront"*/argv[2]))
				break;
		}
		else
		{
			printf("请输入正确指令\n");
			iRet = -1;
			break;
		}
		TranslateVideo::InitImageFrame(&pWriteFrame, CamerDriver.m_pDeCoder->width, CamerDriver.m_pDeCoder->height, AV_PIX_FMT_YUV420P);
		TranslateVideo::InitSwsContext(&pSws, CamerDriver.m_pDeCoder->width, CamerDriver.m_pDeCoder->height, AV_PIX_FMT_YUV422P, pWriteFrame->width, pWriteFrame->height, AV_PIX_FMT_YUV420P);
		{
			
			if (iRet = avformat_alloc_output_context2(&pFmtContext, NULL, "mpegts", filename))
			{
				break;
			}

			m_pVideoStream = avformat_new_stream(pFmtContext, pCodec);
			pVideoCodecCtx = avcodec_alloc_context3(pCodec);
			pVideoCodecCtx->width = CamerDriver.m_pDeCoder->width;
			pVideoCodecCtx->height = CamerDriver.m_pDeCoder->height;
			pVideoCodecCtx->framerate = { 30,1 };
			pVideoCodecCtx->time_base = av_inv_q(pVideoCodecCtx->framerate);
			pVideoCodecCtx->sample_aspect_ratio = CamerDriver.m_pDeCoder->sample_aspect_ratio;
			pVideoCodecCtx->chroma_sample_location = CamerDriver.m_pDeCoder->chroma_sample_location;
			pVideoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
			AVDictionary *pDict = NULL;
			av_dict_set(&pDict, "preset", "ultrafast", 0);
			av_dict_set(&pDict, "tune", "zerolatency", 0);
			//char threadnumstr[10];
			//unsigned int threadnums=std::thread::hardware_concurrency()+1;
			//_itoa(threadnums, threadnumstr, 10);
			//av_dict_set(&pDict, "threads", threadnumstr, 0);
			pVideoCodecCtx->gop_size = 20;
			avcodec_open2(pVideoCodecCtx, pCodec, &pDict);


			avcodec_parameters_from_context(m_pVideoStream->codecpar, pVideoCodecCtx);
			pFmtContext->max_interleave_delta = 0;
			if (!(pFmtContext->flags&AVFMT_NOFILE))
			{
				iRet = avio_open(&pFmtContext->pb, filename, AVIO_FLAG_WRITE);
				if (iRet)
					break;
			}
			//缓存取消
			m_pVideoStream->time_base = pVideoCodecCtx->time_base;
			{
				AVDictionary *pDictFmt = NULL;
				av_dict_set(&pDictFmt, "rtbufsize", "0", 0);
				av_dict_set(&pDictFmt, "start_time_realtime", "0", 0);
				if (iRet = avformat_write_header(pFmtContext, &pDict))
				{
					break;
				}
			}
		}
		realtime = av_gettime();
		while (1)
		{
			CamerDriver.GetFrameFromCamer(&pFrame);			
			pFrame->pts = av_gettime() - realtime;
            //编码时间轴控制
			if (av_rescale_q(pFrame->pts, { 1,AV_TIME_BASE }, pVideoCodecCtx->time_base) <= av_rescale_q(lastpts, m_pVideoStream->time_base, pVideoCodecCtx->time_base))
			{
				av_frame_free(&pFrame);
				continue;
			}
			if (pFrame)
			{
				if (!time)
				{
					time = 19;
					pFrame->pict_type = AV_PICTURE_TYPE_I;
				}
				else
				{
					time--;
					pFrame->pict_type = AV_PICTURE_TYPE_P;
				}
				pFrame->pts = av_rescale_q(pFrame->pts, { 1,AV_TIME_BASE }, m_pVideoStream->time_base);
				iRet=TranslateVideo::EnCodePacket(pSws, m_pVideoStream, m_pVideoStream, pFrame, pWriteFrame, pVideoCodecCtx, &packet,&finsh,&vget);
				if (vget)
				{
					av_write_frame(pFmtContext, &packet);
					vget = 0;
					av_packet_unref(&packet);
				}
				lastpts = pFrame->pts;
				av_frame_free(&pFrame);
                if (iRet)
                    break;
			}
		}
	} while (0);
	return 0;
}
#endif