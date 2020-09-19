#pragma once
#include <interface/base_video_reader.h>

namespace video {
namespace ffmpeg {

class FFmpegVideoReader : public interface::BaseVideoReader {
public:
	FFmpegVideoReader();
	virtual ~FFmpegVideoReader() override;

	DELETE_COPY(FFmpegVideoReader)

	ErrorCode Open() override;
	void Close() override;
	ErrorCode SetVideoConfig(const VideoConfig& video_config) override;
	ErrorCode SetEventCallback(const VideoConfig& video_config) override;
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
	std::shared_ptr<FFmpegVideoReader> operator() () {
		return std::static_pointer_cast<BaseVideoReader>(std::make_shared<FFmpegVideoReader>());
	}
};
}//namespace ffmpeg
}//namespace video
