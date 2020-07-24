#include "pch.h"
#include "Driver.h"
#include <string>
Driver::Driver()
{
    m_pCamer = NULL;
    m_pDeCoder = NULL;
}


Driver::~Driver()
{
    if (m_pCamer)
    {
        avformat_close_input(&m_pCamer);
        //avformat_free_context(m_pCamer);
    }
    if (m_pDeCoder)
        avcodec_free_context(&m_pDeCoder);
}
/*********************************
初始化设备，需要设备名，否则只能在我的电脑用
参数:设备名
返回值:0为正常打开，其他值为错误
备注:
***********************************/
int Driver::Init_camerdriver(const char* dev)
{
    avdevice_register_all();
/*********************************
win32的dshow跟linux的v4l2初始基本相同
***********************************/
#ifdef WIN32
    std::string devstring = "video=";
    const char* testdev = "Integrated Webcam";
#elif LINUX
	std::string devstring = "";
	const char* testdev = "/dev/video0";
#endif
    int iRet = 0;
	//摄像头的格式封装，主要存储摄像头信息跟摄像头数据读取函数
    AVFormatContext*    pFmtCtx = NULL;		
	//摄像头的文件IO流，与pFormat->pb联系密切
    AVInputFormat*      pInputFmt = NULL;		
	//摄像头图片解码器，用于解码MJPEG流
    AVCodecContext*     pCamerDeCoder = NULL;		

    if (!dev)
        dev = testdev;
    devstring += dev;
    do {
        pFmtCtx = avformat_alloc_context();
#ifdef WIN32
        pInputFmt = av_find_input_format("dshow");
#elif LINUX
		pInputFmt = av_find_input_format("video4linux2");
#endif
		//打开并连接摄像头
		AVDictionary *pdict=NULL; 
		av_dict_set(&pdict, "rtbufsize", "100", 0);
		av_dict_set(&pdict, "start_time_realtime", "0", 0);
		if (pInputFmt&&pFmtCtx)
			iRet = avformat_open_input(&pFmtCtx, devstring.c_str(), pInputFmt, &pdict);
		else
			iRet = -1;
        if (iRet)
            break;
		//打印摄像头信息
		av_dump_format(pFmtCtx, 0, testdev, 0);
		//根据摄像头信息初始化解码器
        pCamerDeCoder = avcodec_alloc_context3(avcodec_find_decoder(pFmtCtx->streams[0]->codecpar->codec_id));
        avcodec_parameters_to_context(pCamerDeCoder, pFmtCtx->streams[0]->codecpar);
		//打开解码器
        iRet=avcodec_open2(pCamerDeCoder, avcodec_find_decoder(pFmtCtx->streams[0]->codecpar->codec_id), NULL);
        if (iRet)
            break;

        m_pCamer = pFmtCtx;
        m_pDeCoder = pCamerDeCoder;
    } while (0);
    if (iRet)
    {
        if(pFmtCtx)
            avformat_free_context(pFmtCtx);
        if (pCamerDeCoder)
            avcodec_free_context(&pCamerDeCoder);
    }
    return iRet;
}

int Driver::Init_screendriver()
{
    avdevice_register_all();
    const char* str = "desktop";
    int iRet = 0;
    AVFormatContext*    pFmtCtx = NULL;
    AVInputFormat*      pInputFmt = NULL;
    AVCodecContext*     pCamerDeCoder = NULL;

    do {
        pFmtCtx = avformat_alloc_context();
        pInputFmt = av_find_input_format("gdigrab");
        if (pInputFmt&&pFmtCtx)
            iRet = avformat_open_input(&pFmtCtx, str, pInputFmt, NULL);
        if (iRet)
            break;

        pCamerDeCoder = avcodec_alloc_context3(avcodec_find_decoder(pFmtCtx->streams[0]->codecpar->codec_id));
        avcodec_parameters_to_context(pCamerDeCoder, pFmtCtx->streams[0]->codecpar);
        iRet = avcodec_open2(pCamerDeCoder, avcodec_find_decoder(pFmtCtx->streams[0]->codecpar->codec_id), NULL);
        if (iRet)
            break;

        m_pCamer = pFmtCtx;
        m_pDeCoder = pCamerDeCoder;
    } while (0);
    if (iRet)
    {
        if (pFmtCtx)
            avformat_free_context(pFmtCtx);
        if (pCamerDeCoder)
            avcodec_free_context(&pCamerDeCoder);
    }
    return iRet;
}

int Driver::GetFrameFromCamer(AVFrame ** frame)
{
    AVPacket packet;
    av_init_packet(&packet);
    AVFrame *tmpframe=av_frame_alloc();
    int iRet = 0;
    do 
    {
        iRet = av_read_frame(m_pCamer, &packet);
        packet.flags = AV_PKT_FLAG_KEY;
        if (iRet)
            break;

        iRet=avcodec_send_packet(m_pDeCoder, &packet);
        if (iRet)
            break;
        iRet = avcodec_receive_frame(m_pDeCoder, tmpframe);
        if (iRet)
            break;
        *frame = tmpframe;
    } while (0);    
    av_packet_unref(&packet);
    if (iRet)
    {
        if (tmpframe)
            av_frame_free(&tmpframe);
    }
    return iRet;
}

int Driver::Frame_2Picture(const char * filename, AVFrame * pFrame, AVCodecID EnCodeID)
{
    AVFormatContext *pPictureFmt = NULL;
    AVStream *pOutStream = NULL;
    AVPacket Packet;
    AVCodec *pCodec = NULL;
    AVCodecContext *pPictureEnCoder = NULL;
    AVFrame *pOutFrame = NULL;
    SwsContext *pSwsCtx = NULL;
    int iRet = 0;
    do {
        iRet = avformat_alloc_output_context2(&pPictureFmt, NULL, NULL, filename);
        if (iRet < 0)
        {
            printf("can't open output\n %s %d", __FILE__, __LINE__);
            break;
        }
        if (pPictureFmt->oformat->video_codec != AV_CODEC_ID_NONE)
        {
            if (EnCodeID == AV_CODEC_ID_NONE)
                EnCodeID = pPictureFmt->oformat->video_codec;
            pCodec = avcodec_find_encoder(EnCodeID);
            if (pCodec)
            {
                pOutStream = avformat_new_stream(pPictureFmt, pCodec);
                pPictureEnCoder = avcodec_alloc_context3(pCodec);
                pPictureEnCoder->width = pFrame->width;
                pPictureEnCoder->height = pFrame->height;
                pPictureEnCoder->time_base = { 1,30 };
                pPictureEnCoder->pix_fmt = AV_PIX_FMT_BGRA;
                if (pCodec->pix_fmts)
                {
                    pPictureEnCoder->pix_fmt = pCodec->pix_fmts[0];
                    for (int i = 0; pCodec->pix_fmts[i] != AV_PIX_FMT_NONE; i++)
                        if (pCodec->pix_fmts[i] == pFrame->format)
                        {
                            pPictureEnCoder->pix_fmt = pCodec->pix_fmts[i];
                            break;
                        }
                }
                if (pPictureFmt->oformat->flags&AVFMT_GLOBALHEADER)
                    pPictureEnCoder->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                iRet = avcodec_open2(pPictureEnCoder, pCodec, NULL);
                if (!iRet)
                {
                    iRet = avcodec_parameters_from_context(pOutStream->codecpar, pPictureEnCoder);
                }
                else
                {
                    printf("can't open picturecodectx\n%s %d\n", __FILE__, __LINE__);
                    break;
                }
                pOutStream->time_base = pPictureEnCoder->time_base;
            }
            else
            {
                printf("Stream create error\n%s %d\n", __FILE__, __LINE__);
                iRet = -1;
                break;
            }
        }
        if (pFrame->format != pPictureEnCoder->pix_fmt) {
            iRet = InitImageFrame(&pOutFrame, pFrame->width, pFrame->height, (AVPixelFormat)pPictureEnCoder->pix_fmt);
            if (iRet)
                break;
            pSwsCtx = sws_getContext(pFrame->width, pFrame->height, (AVPixelFormat)pFrame->format
                , pOutFrame->width, pOutFrame->height, pPictureEnCoder->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
            if (!pSwsCtx)
            {
                iRet = -1;
                break;
            }
            if (iRet = (!sws_scale(pSwsCtx, pFrame->data, pFrame->linesize, 0,
                pFrame->height, pOutFrame->data, pOutFrame->linesize)))
            {
                printf("convert video frame error\n%s %d\n", __FILE__, __LINE__);
                break;
            }
        }
        else
            pOutFrame = pFrame;
        if (!(pPictureFmt->flags&AVFMT_NOFILE))
        {
            iRet = avio_open(&pPictureFmt->pb, filename, AVIO_FLAG_WRITE);
            if (iRet)
                break;
        }
        iRet = avformat_write_header(pPictureFmt, NULL);
        if (iRet)
        {
            printf("can't not write header\n%s %d\n", __FILE__, __LINE__);
            break;
        }
        av_init_packet(&Packet);
        iRet = avcodec_send_frame(pPictureEnCoder, pOutFrame);
        if (iRet != 0)
        {
            break;
        }
        do {
            iRet = avcodec_receive_packet(pPictureEnCoder, &Packet);
            if (iRet != 0)
            {
                if (iRet == AVERROR_EOF)
                    break;
                avcodec_send_frame(pPictureEnCoder, NULL);
                avcodec_flush_buffers(pPictureEnCoder);
            }
            else
                break;
        } while (true);
        iRet = av_write_frame(pPictureFmt, &Packet);
        av_packet_unref(&Packet);
        if (iRet)
            break;

        iRet = av_write_trailer(pPictureFmt);
        if (iRet)
            break;
    } while (0);
    if (pSwsCtx)
        sws_freeContext(pSwsCtx);
    if (pPictureEnCoder)
        avcodec_free_context(&pPictureEnCoder);
    if (pPictureFmt)
    {
        avio_closep(&pPictureFmt->pb);
        avformat_free_context(pPictureFmt);
    }
    if (pOutFrame != pFrame)
    {
        av_freep(&pOutFrame->data[0]);
        av_frame_free(&pOutFrame);
    }
    return iRet;
}

int Driver::InitImageFrame(AVFrame ** frame, int width, int height, AVPixelFormat pix_fmt)
{
    AVFrame *pframe = NULL;
    int iRet = 0;
    do {
        if (!(pframe = av_frame_alloc())) {
            fprintf(stderr, "Could not allocate output frame\n %s %d", __FILE__, __LINE__);
            iRet = -1;
            break;
        }
        pframe->width = width;
        pframe->height = height;
        pframe->format = pix_fmt;
        pframe->pts = 0;
        iRet = av_image_alloc(
            pframe->data,
            pframe->linesize,
            pframe->width,
            pframe->height,
            pix_fmt,
            4
        );
        if (iRet < 0)
            break;
        else
            iRet = 0;
    } while (0);
    *frame = pframe;
    return iRet;
}
