#include "pch.h"
#include "TranslateAudio.h"
/* The output bit rate in bit/s */
#define OUTPUT_BIT_RATE 96000
/* The number of output channels */
#define OUTPUT_CHANNELS 2
extern char g_sError_ptr[AV_ERROR_MAX_STRING_SIZE];
extern char g_sError_where[1024];
TranslateAudio::TranslateAudio()
{
}

TranslateAudio::~TranslateAudio()
{
}


int TranslateAudio::DeCodeframe(AVCodecContext *input_codec_context IN, AVPacket *audio_packet IN, AVFrame **frame OUT, int *finsh OUT)
{
    int iRet = 0;
    AVFrame *audio_frame=NULL;
    //audio_frame = av_frame_alloc();
    do {
        iRet = Init_input_frame(&audio_frame);
        if (iRet)
            break;
        iRet = avcodec_send_packet(input_codec_context, audio_packet);       
        if (iRet == AVERROR_INVALIDDATA)
        {
            iRet = 0;
            break;
        }
        else if (iRet < 0)
        {
            sprintf_s(g_sError_where, "error send audio packet\n%s %d\n",__FILE__, __LINE__);
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
            break;
        }
        iRet = avcodec_receive_frame(input_codec_context, audio_frame);
        if (iRet == AVERROR(EAGAIN))
        {
            av_frame_free(&audio_frame);
            iRet = 0;
        }
        else if (iRet == AVERROR_EOF)
        {
            *finsh = 1;
            iRet = 0;
        }
        else if(iRet<0){
            sprintf_s(g_sError_where, "decode audio error\n%s %d\n", __FILE__, __LINE__);
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
        }
    } while (0);
    av_packet_unref(audio_packet);
    *frame = audio_frame;
    return iRet;
}



int TranslateAudio::FlushPacket(AVStream *audio_stream IN, AVFormatContext *output_fmt_context IN, AVCodecContext *output_codec_context OUT, AVPacket *audio_packet IN)
{
    int iRet = 0;    
    int finsh = 0;    
    while (!finsh)
    {
        bool bIsPacket = false;
        do {
            //av_init_packet(audio_packet);
            iRet = avcodec_send_frame(output_codec_context, NULL);
            if (iRet == AVERROR_EOF) {
                finsh = 1;
                iRet = 0;
                break;
            }
            if (iRet < 0)
            {
                
                sprintf_s(g_sError_where, "error send audio packet\n%s %d\n", __FILE__, __LINE__);
                av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
                break;
            }
            iRet = avcodec_receive_packet(output_codec_context, audio_packet);
            avcodec_flush_buffers(output_codec_context);
            if (iRet == AVERROR(EAGAIN))
            {
                iRet = 0;
                break;
            }
            else if (iRet == AVERROR_EOF)
            {
                finsh = 1;
                iRet = 0;
                break;
            }
            else if (iRet < 0) {
                av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
                sprintf_s(g_sError_where, "decode audio error\n%s %d\n", __FILE__, __LINE__);
                break;
            }
            bIsPacket = true;
        } while (0);
        if (bIsPacket)
        {
            audio_packet->stream_index = audio_stream->index;
            av_packet_rescale_ts(audio_packet, output_codec_context->time_base, audio_stream->time_base);
            iRet=av_interleaved_write_frame(output_fmt_context, audio_packet);
        }
        av_packet_unref(audio_packet);
        if (iRet < 0)
            break;
    }
    return iRet;
}




int TranslateAudio::Init_input_frame(AVFrame ** frame IN)
{
    if (!(*frame = av_frame_alloc())) {
        sprintf_s(g_sError_where, "Could not allocate input frame\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        return AVERROR(ENOMEM);
    }
    return 0;
}

int TranslateAudio::Init_resampler(AVCodecContext * input_codec_context IN, AVCodecContext * output_codec_context IN, SwrContext ** resample_context OUT)
{
    int error;

    /*
     * Create a resampler context for the conversion.
     * Set the conversion parameters.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity (they are sometimes not detected
     * properly by the demuxer and/or m_pthreadencoder).
     */
    *resample_context = swr_alloc_set_opts(NULL,
        av_get_default_channel_layout(output_codec_context->channels),
        output_codec_context->sample_fmt,
        output_codec_context->sample_rate,
        av_get_default_channel_layout(input_codec_context->channels),
        input_codec_context->sample_fmt,
        input_codec_context->sample_rate,
        0, NULL);
    if (!*resample_context) {
        sprintf_s(g_sError_where, "Could not allocate resample context\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        return AVERROR(ENOMEM);
    }
    /*
    * Perform a sanity check so that the number of converted samples is
    * not greater than the number of samples to be converted.
    * If the sample rates differ, this case has to be handled differently
    */
    //这里被修改了,可能会出问题
    //av_assert0(output_codec_context->sample_rate == input_codec_context->sample_rate);

    /* Open the resampler with the specified parameters. */
    if ((error = swr_init(*resample_context)) < 0) {
        sprintf_s(g_sError_where, "Could not open resample context\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, error);
        swr_free(resample_context);
        return error;
    }
    return 0;
}

int TranslateAudio::Init_fifo(AVAudioFifo ** fifo, AVCodecContext * output_codec_context)
{
    /* Create the FIFO buffer based on the specified output sample format. */
    if (!(*fifo = av_audio_fifo_alloc(output_codec_context->sample_fmt,
        output_codec_context->channels, 1))) {
        sprintf_s(g_sError_where, "Could not allocate FIFO\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        return AVERROR(ENOMEM);
    }
    return 0;
}


int TranslateAudio::Init_converted_samples(uint8_t *** converted_input_samples OUT, AVCodecContext * output_codec_context IN, int frame_size)
{
    int error;

    /* Allocate as many pointers as there are audio channels.
     * Each pointer will later point to the audio samples of the corresponding
     * channels (although it may be NULL for interleaved formats).
     */
    if (!(*converted_input_samples =(uint8_t **)calloc(output_codec_context->channels,
        sizeof(**converted_input_samples)))) {
        sprintf_s(g_sError_where, "Could not allocate converted input sample pointers\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        return AVERROR(ENOMEM);
    }

    /* Allocate memory for the samples of all channels in one consecutive
     * block for convenience. */
    if ((error = av_samples_alloc(*converted_input_samples, NULL,
        output_codec_context->channels,
        frame_size,
        output_codec_context->sample_fmt, 0)) < 0) {
        sprintf_s(g_sError_where, "Could not allocate converted input samples\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, error);

        av_freep(&(*converted_input_samples)[0]);
        free(*converted_input_samples);
        return error;
    }
    return 0;
}

int TranslateAudio::Convert_samples(const uint8_t ** input_data, uint8_t ** converted_data, const int input_size, const int output_size, SwrContext * resample_context)
{
    int error;
    //超过内存允许
    if (output_size > 1024 * 1024)
    {
        sprintf_s(g_sError_where, "frame_size overload that member\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        error = -2;
        return error;
    }
    /* Convert the samples using the resampler. */
    if ((error = swr_convert(resample_context,
        converted_data, output_size,
        input_data, input_size)) < 0) {
        sprintf_s(g_sError_where, "Could not convert input samples\n%s %d\n", __FILE__, __LINE__);        
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, error);
        return error;
    }

    return 0;
}

int TranslateAudio::Add_samples_to_fifo(AVAudioFifo * fifo, uint8_t ** converted_input_samples, const int frame_size)
{
    int error;

    /* Make the FIFO as large as it needs to be to hold both,
     * the old and the new samples. */
    int size = av_audio_fifo_size(fifo);
    if ((error = av_audio_fifo_realloc(fifo, size + frame_size)) < 0) {
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, error);
        sprintf_s(g_sError_where, "Could not reallocate FIFO\n%s %d\n", __FILE__, __LINE__);
        return error;
    }

    /* Store the new samples in the FIFO buffer. */
    if (av_audio_fifo_write(fifo, (void **)converted_input_samples,
        frame_size) < frame_size) {
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR_EXIT);
        sprintf_s(g_sError_where, "Could not write data to FIFO\n%s %d\n", __FILE__, __LINE__);
        return AVERROR_EXIT;
    }
    //size=av_audio_fifo_space(fifo);
    return 0;
}

int TranslateAudio::InsertEmptyVoice(AVFormatContext * pFmtCtxDest, AVCodecContext *AudioCodeCtx, AVStream * pStreamDest, int64_t *pPtsStart, int64_t iPtsEnd)
{
    AVCodecContext* pCodecCtx = AudioCodeCtx;
    AVPacket    packet;
    AVFrame*    pFrameOut = NULL;
    int64_t iStartpts = *pPtsStart;
    int afinsh=0;
    int iRet = 0;
    if (!pFmtCtxDest || !pStreamDest||!AudioCodeCtx)
        iRet;
    do {
        pFrameOut = av_frame_alloc();
        if (!pFrameOut)
        {
            iRet = -1;
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
            sprintf_s(g_sError_where, "can't alloc audio frame\n%s %d", __FILE__, __LINE__);
        }
        pFrameOut->nb_samples = pCodecCtx->frame_size;
        pFrameOut->format = pCodecCtx->sample_fmt;
        pFrameOut->channel_layout = pCodecCtx->channel_layout;
        pFrameOut->channels = pCodecCtx->channels;
        pFrameOut->sample_rate = pCodecCtx->sample_rate;
        iRet=av_frame_get_buffer(pFrameOut, 0);
        if (iRet)
        {
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
            sprintf_s(g_sError_where, "can't alloc frame audio buffer\n%s %d", __FILE__, __LINE__);            
            break;
        }
        iRet=av_samples_set_silence(pFrameOut->data, 0, pFrameOut->nb_samples, pFrameOut->channels, (AVSampleFormat)pFrameOut->format);
        if (iRet)
        {
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
            sprintf_s(g_sError_where, "can't set frame audio buffer\n%s %d", __FILE__, __LINE__);
            break;
        }
        //编码每一帧的字节数
        Init_packet(&packet);
        while (true)
        {
            if (iStartpts >= iPtsEnd|| iRet)
                break;     
            pFrameOut->pts = iStartpts;
            iStartpts += pFrameOut->nb_samples;
            do{
                iRet = avcodec_send_frame(AudioCodeCtx, pFrameOut);
                if (iRet == AVERROR_EOF) {
                    afinsh = 1;
                    iRet = 0;
                    break;
                }
                else if (iRet < 0)
                {
                    av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
                    sprintf_s(g_sError_where, "error send audio packet\n%s %d\n", __FILE__, __LINE__);
                    break;
                }
                iRet = avcodec_receive_packet(AudioCodeCtx, &packet);
                if (iRet == AVERROR(EAGAIN))
                {
                    iRet = 0;
                    break;
                }
                else if (iRet == AVERROR_EOF)
                {
                    afinsh = 1;
                    iRet = 0;
                    break;
                }
                else if (iRet < 0) {
                    av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
                    sprintf_s(g_sError_where, "decode audio error\n%s %d\n", __FILE__, __LINE__);
                    break;
                }
            } while (0);
            if (iRet|| afinsh)
                break;
            av_packet_rescale_ts(&packet, AudioCodeCtx->time_base, pStreamDest->time_base);
            packet.stream_index = pStreamDest->index;
            iRet=av_interleaved_write_frame(pFmtCtxDest, &packet);
            av_packet_unref(&packet);
        }        
    } while (0);
    if (pFrameOut)
        av_frame_free(&pFrameOut);
    *pPtsStart = iStartpts;
    return iRet;
}


int TranslateAudio::Init_output_frame(AVFrame ** frame, AVCodecContext * output_codec_context, int frame_size)
{
    int error;

    /* Create a new frame to store the audio samples. */
    if (!(*frame = av_frame_alloc())) {
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, AVERROR(ENOMEM));
        sprintf_s(g_sError_where, "Could not allocate output frame\n%s %d\n", __FILE__, __LINE__);
        return AVERROR_EXIT;
    }

    /* Set the frame's parameters, especially its size and format.
     * av_frame_get_buffer needs this to allocate memory for the
     * audio samples of the frame.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity. */
    (*frame)->nb_samples = frame_size;
    (*frame)->channel_layout = output_codec_context->channel_layout;
    (*frame)->format = output_codec_context->sample_fmt;
    (*frame)->sample_rate = output_codec_context->sample_rate;

    /* Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified. */
    if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
        sprintf_s(g_sError_where, "Could not allocate output frame samples\n%s %d\n", __FILE__, __LINE__);
        av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, error);
        av_frame_free(frame);
        return error;
    }

    return 0;
}

void TranslateAudio::Init_packet(AVPacket * packet)
{
    av_init_packet(packet);
    /* Set the packet data and size so that it is recognized as being empty. */
    packet->data = NULL;
    packet->size = 0;
}



int TranslateAudio::EnCodePacket(AVAudioFifo * fifo IN, AVStream *audio_stream IN, AVCodecContext * output_codec_context IN, AVPacket * Packet OUT,int64_t *Pts IN OUT, int *getpacket OUT)
{
    AVFrame *output_frame;
    /* Packet used for temporary storage. */
    int iRet=0;
    /* Use the maximum number of possible samples per frame.
     * If there is less than the maximum possible frame size in the FIFO
     * buffer use this number. Otherwise, use the maximum possible frame size. */
    const int frame_size = FFMIN(av_audio_fifo_size(fifo),
        output_codec_context->frame_size);

    /* Initialize temporary storage for one output frame. */
    if (Init_output_frame(&output_frame, output_codec_context, frame_size))
        return AVERROR_EXIT;

    /* Read as many samples from the FIFO buffer as required to fill the frame.
     * The samples are stored in the frame temporarily. */
    if (av_audio_fifo_read(fifo, (void **)output_frame->data, frame_size) < frame_size) {
        sprintf_s(g_sError_ptr, "Could not read data from FIFO\n");
        sprintf_s(g_sError_where, "Could not read data from FIFO\n%s %d\n", __FILE__, __LINE__);
        av_frame_free(&output_frame);
        return AVERROR_EXIT;
    }
    Init_packet(Packet);
    do {
        /* Set a timestamp based on the sample rate for the container. */
        if (output_frame) {
            output_frame->pts = *Pts;
            *Pts += output_frame->nb_samples;
        }

        /* Send the audio frame stored in the temporary packet to the encoder.
         * The output audio stream encoder is used to do this. */
        iRet = avcodec_send_frame(output_codec_context, output_frame);
        av_frame_free(&output_frame);
        /* The encoder signals that it has nothing more to encode. */
        if (iRet == AVERROR_EOF) {
            iRet = 0;
            break;
        }
        else if (iRet < 0) {
            sprintf_s(g_sError_where, "Could not send packet for encoding\n%s %d\n", __FILE__, __LINE__);
            av_make_error_string(g_sError_ptr, AV_ERROR_MAX_STRING_SIZE, iRet);
            break;
        }

        /* Receive one encoded frame from the encoder. */
        iRet = avcodec_receive_packet(output_codec_context, Packet);
        /* If the encoder asks for more data to be able to provide an
         * encoded frame, return indicating that no data is present. */
        if (iRet == AVERROR(EAGAIN)) {
            iRet = 0;
            break;
            /* If the last frame has been encoded, stop encoding. */
        }
        else if (iRet == AVERROR_EOF) {
            iRet = 0;
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
            if (Packet->pts < 0)
                break;
            av_packet_rescale_ts(Packet, output_codec_context->time_base, audio_stream->time_base);
            Packet->stream_index = audio_stream->index;
            *getpacket = 1;
        }
    } while (0);
    return iRet;
}

