#include "pch.h"
#include "TranslateVideo.h"
extern char g_sError_ptr[AV_ERROR_MAX_STRING_SIZE];
extern char g_sError_where[1024];
TranslateVideo::TranslateVideo()
{
}


TranslateVideo::~TranslateVideo()
{
}

int TranslateVideo::DeCodeframe(
    AVCodecContext * input_codec_context IN,
    AVPacket * video_packet IN,
    AVFrame ** frame OUT,
    int *finsh OUT)
{
    int iRet = 0;
    AVFrame *video_frame = NULL;
    do {
        iRet = init_input_frame(&video_frame);
        //iRet = InitImageFrame(&video_frame, input_codec_context->width, input_codec_context->height, input_codec_context->pix_fmt);
        if (iRet)
            break;
        iRet = avcodec_send_packet(input_codec_context, video_packet);
        if (iRet == AVERROR_EOF) {
            *finsh = 1;
            iRet = 0;
            break;
        }
        else if (iRet == AVERROR_INVALIDDATA)
        {
            //暂时无法处理，找不到资料            
            iRet = 0;
            av_frame_free(&video_frame);
            break;
        }
        if (iRet < 0)
        {
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
            sprintf_s(g_sError_where, "error send video packet\n%s %d\n", __FILE__, __LINE__);
            break;
        }
        iRet = avcodec_receive_frame(input_codec_context, video_frame);
        if (iRet == AVERROR(EAGAIN))
        {
            av_frame_free(&video_frame);
            iRet = 0;
        }
        else if (iRet == AVERROR_EOF)
        {
            av_frame_free(&video_frame);
            *finsh = 1;
            iRet = 0;
        }
        else if (iRet < 0) {
            av_frame_free(&video_frame);
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
            sprintf_s(g_sError_where, "decode video error\n%s %d\n", __FILE__, __LINE__);
        }
    } while (0);
    av_packet_unref(video_packet);
    *frame = video_frame;
    return iRet;
}

int TranslateVideo::InitSwsContext(SwsContext **convert_context OUT, int input_width, int input_height, AVPixelFormat input_format, int ouput_width, int ouput_height, AVPixelFormat ouput_format)
{
    SwsContext *sws=NULL;
    sws = sws_getContext(
        input_width, input_height, input_format,
        ouput_width, ouput_height, ouput_format,
        SWS_BICUBIC, NULL, NULL, NULL
    );
    if (sws == NULL)
        return -1;
    *convert_context = sws;
    return 0;
}

int TranslateVideo::InitImageFrame(AVFrame **frame OUT, int width, int height, AVPixelFormat pix_fmt)
{
    AVFrame *pframe=NULL;
    int iRet = 0;
    do {
        if (!(pframe = av_frame_alloc())) {
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
            sprintf_s(g_sError_where, "Could not allocate output frame\n %s %d", __FILE__, __LINE__);
            iRet = -1;
            break;
        }
        pframe->width = width;
        pframe->height = height;
        pframe->format = pix_fmt;
        pframe->pts = 0;
        iRet=av_image_alloc(
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
    *frame=pframe;
    return iRet;
}

int TranslateVideo::EnCodePacket(
    SwsContext *swsctx IN, 
    AVStream *VideoInStream IN,
    AVStream *VideoOutStream IN,
    AVFrame *Inputframe IN, 
    AVFrame *Outputframe IN,
    AVCodecContext *output_codec_context IN,
    AVPacket *video_packet OUT, 
    int *finsh OUT, 
    int *getpacket OUT)
{
    int iRet=0;
    do {        
        if (Outputframe&&Inputframe && swsctx)
        {
            if (iRet = (!sws_scale(swsctx, Inputframe->data, Inputframe->linesize, 0,
                Inputframe->height, Outputframe->data, Outputframe->linesize)))
            {
                sprintf_s(g_sError_ptr,"convert video frame error\n");
                sprintf_s(g_sError_where, "convert video frame error\n%s %d\n", __FILE__, __LINE__);
                break;
            }
            Outputframe->pts = Inputframe->pts;
            Outputframe->pict_type = Inputframe->pict_type;
        }
        //用于给codec控制码率...
        if(Outputframe)
            Outputframe->pts=av_rescale_q(Outputframe->pts, VideoInStream->time_base, output_codec_context->time_base);
        iRet = avcodec_send_frame(output_codec_context, Outputframe);
        //if (Inputframe)
        //{
        //    Outputframe = Inputframe;
        //    Outputframe->pict_type = AV_PICTURE_TYPE_NONE;
        //    iRet = avcodec_send_frame(output_codec_context, Outputframe);
        //}
        if (iRet == AVERROR_EOF) {
            iRet = 0;
            *finsh = 1;
            break;
        }
        else if (iRet == AVERROR_INVALIDDATA)
        {
            //暂时无法处理，找不到资料            
            iRet = 0;            
            break;
        }
        else if (iRet < 0) {
            sprintf_s(g_sError_where, "Could not send packet for encoding\n%s %d\n", __FILE__, __LINE__);
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
            break;
        }

        /* Receive one encoded frame from the encoder. */
        iRet = avcodec_receive_packet(output_codec_context, video_packet);
        /* If the encoder asks for more data to be able to provide an
         * encoded frame, return indicating that no data is present. */
        if (iRet == AVERROR(EAGAIN)) {
            iRet = 0;
            break;
            /* If the last frame has been encoded, stop encoding. */
        }
        else if (iRet == AVERROR_EOF) {
            iRet = 0;
            *finsh = 1;
            break;
        }
        else if (iRet < 0) {
            sprintf_s(g_sError_where, "Could not encode frame\n%s %d\n", __FILE__, __LINE__);
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
            break;
            /* Default case: Return encoded data. */
        }
        else
        {
            av_packet_rescale_ts(video_packet, output_codec_context->time_base, VideoOutStream->time_base);
            video_packet->stream_index=VideoOutStream->index;            
            *getpacket = 1;
        }
            
    } while (0);
    return iRet;
}

int TranslateVideo::init_input_frame(AVFrame ** frame)
{
    if (!(*frame = av_frame_alloc())) {
        sprintf_s(g_sError_where, "Could not allocate input frame\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        return -1;
    }
    return 0;
}

int TranslateVideo::FlushPacket(AVStream *VideoOutStream IN, AVFormatContext *OutputFmt IN,AVCodecContext *VideoEnCodeCtx IN)
{
    int iRet = 0;
    AVPacket output_packet;
    int vfinsh = 0, vget = 0;
    av_init_packet(&output_packet);
    while (!(iRet = EnCodePacket(NULL, VideoOutStream, VideoOutStream, NULL, NULL, VideoEnCodeCtx, &output_packet, &vfinsh, &vget)))
    {
        if (vfinsh)
            break;
        if (vget)
        {
            iRet = av_interleaved_write_frame(OutputFmt, &output_packet);
            av_packet_unref(&output_packet);
            if (iRet)
                break;
        }
        avcodec_flush_buffers(VideoEnCodeCtx);
        //说明刷不出数据了，源视频有问题，应该放弃继续刷数据
        if (vget == 0)
            break;
        vget = 0;
    }
    return iRet;
}
