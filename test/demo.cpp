#include <glog/logging.h>
#include "frame.h"
#include "video_capture.h"
#include "ffmpeg/ffmpeg_video_reader.h"
#include "ffmpeg/ffmpeg_video_decoder.h"

void callback(int channel_id, const video::Frame& frame) {
	LOG(ERROR) << "receive frame channel_id:" << channel_id;
}

int main() {
	video::VideoCapture video_capture;
	video_capture.SetVideoReaderCreator(video::ffmpeg::FFmpegVideoReaderCreator());
	video_capture.SetVideoDecoderCreator(video::ffmpeg::FFmpegVideoDecoderCreator());

	video::VideoConfig video_config;
	video_config["url"] = "./zhuheqiao.h264";
	video_config["rtsp_transport"] = "tcp";
	video_config["max_delay"] = "1000000";
	video_config["stimeout"] = "40000";
	video_config["buffer_size"] = "1048576";
	video_config["pump_frame_rate"] = "3";

	int channel_count = 1;
	
	for (int i = 0; i < channel_count; ++i) {
		video_capture.SetChannelVideoConfig(i, video_config);
		video_capture.SetChannelFrameCallback(i, std::bind(callback, i, std::placeholders::_1));
		video_capture.StartChannel(i);
	}

	LOG(INFO) << "start finish";

	std::this_thread::sleep_for(std::chrono::seconds(60));

	video_capture.StopAllChannel();

	return 0;
}
