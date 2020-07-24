#pragma once
extern "C"
{
#include <libavformat/avformat.h>
}
#define SAVING true
#define WAITING false
class SaveToFile
{
public:
	SaveToFile();
	~SaveToFile();
	int Save(bool type, AVPacket *Packet, AVCodecContext *Coder,const char *filename);
private:
	AVFormatContext* m_pFilefmt;
	int64_t m_iStartTime;
	AVStream* m_pVideoStream;
	bool m_iType;
	bool m_bStatus;
};

