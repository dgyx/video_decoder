#pragma once

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#endif

#include <interface/base_video_reader.h>

namespace video {
namespace ffmpeg {

class FFmpegVideoReader : public BaseVideoReader {
public:
	FFmpegVideoReader();
	virtual ~FFmpegVideoReader() override;

	DELETE_COPY(FFmpegVideoReader)

	ErrorCode Open() override;
	void Close() override;
	ErrorCode ReceivePacket(Packet& packet) override;

private:
	void Release();

private:
	AVDictionary* av_options_;
	AVFormatContext* av_format_context_;
	AVBSFContext* mp4toannexb_avbsf_;
	AVBSFContext* dump_extra_avbsf_;
	int video_stream_index_;
};

struct FFmpegVideoReaderCreator {
	std::shared_ptr<BaseVideoReader> operator() () {
		return std::static_pointer_cast<BaseVideoReader>(std::make_shared<FFmpegVideoReader>());
	}
};
}//namespace ffmpeg
}//namespace video
