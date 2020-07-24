#include "pch.h"
#include "SaveToFile.h"
SaveToFile::SaveToFile()
{
	m_pFilefmt = NULL;
	m_iType = WAITING;
	m_bStatus = false;
}

SaveToFile::~SaveToFile()
{
}
//该模块后续测试发现有时会花屏，原因暂时未找到。
int SaveToFile::Save(bool type, AVPacket *Packet, AVCodecContext * pCoder, const char * filename)
{
	int iRet = 0;
	const AVCodec* codec = avcodec_find_encoder(pCoder->codec_id);
	do {
		if (m_iType == WAITING && type == SAVING)
		{
			if (iRet = avformat_alloc_output_context2(&m_pFilefmt, NULL, NULL, filename))
			{
				break;
			}
			m_pVideoStream=avformat_new_stream(m_pFilefmt, pCoder->codec);
			avcodec_parameters_from_context(m_pVideoStream->codecpar, pCoder);
			if (!(m_pFilefmt->flags&AVFMT_NOFILE))
			{
				iRet = avio_open(&m_pFilefmt->pb, filename, AVIO_FLAG_WRITE);
				if (iRet)
					break;
			}
			m_pVideoStream->time_base = pCoder->time_base;
			m_pVideoStream->codecpar->codec_tag = 0;
			if (iRet = avformat_write_header(m_pFilefmt, NULL))
			{
				break;
			}
			m_iType=type;	
		}
		if (type == SAVING)
		{
			if (!m_bStatus)
			{
				//关键帧
				if (Packet->flags == AV_PKT_FLAG_KEY)
				{
					m_iStartTime= Packet->pts;
					m_bStatus = true;
				}
			}
			if (m_bStatus)
			{
				Packet->stream_index = m_pVideoStream->index;
				Packet->pts = Packet->pts - m_iStartTime;
				Packet->dts = Packet->dts - m_iStartTime;
				av_write_frame(m_pFilefmt, Packet);
			}
		}
		if (m_iType == SAVING && type == WAITING)
		{
			av_write_trailer(m_pFilefmt);
			avio_closep(&m_pFilefmt->pb);
			avformat_free_context(m_pFilefmt);
			m_bStatus = false;
			m_iType = WAITING;
		}
	} while (0);
	return iRet;
}

