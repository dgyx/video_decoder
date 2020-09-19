#pragma once

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#endif

#include "interface/base_video_decoder.h"


namespace video {
namespace ffmpeg {

class FFmpegVideoDecoder : public BaseVideoDecoder {
public:
	FFmpegVideoDecoder();
	virtual ~FFmpegVideoDecoder() override;

	DELETE_COPY(FFmpegVideoDecoder)

	ErrorCode SendPacket(const Packet& packet) override;
private:
	AVCodecContext* av_codec_context_;
	SwsContext* sws_context_;
	int current_frame_id_;
};

struct FFmpegVideoDecoderCreator {
	std::shared_ptr<BaseVideoDecoder> operator() () {
		return std::static_pointer_cast<BaseVideoDecoder>(std::make_shared<FFmpegVideoDecoder>());
	}
};
}//namespace ffmpeg
}//namespace video
