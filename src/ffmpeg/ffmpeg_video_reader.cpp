#include <cstring>
#include <glog/logging.h>
#include "ffmpeg/ffmpeg_video_reader.h"

namespace video {
namespace ffmpeg {

FFmpegVideoReader::FFmpegVideoReader() {
	av_register_all();
	avformat_network_init();
}

FFmpegVideoReader::~FFmpegVideoReader() {
	Release();
}

ErrorCode FFmpegVideoReader::Open() {
	std::lock_guard<std::recursive_mutex> lock(mutex_);
	if (NULL != av_format_context_) {
		LOG(ERROR) << "video reader has been opened:";
		return ErrorCode::kReaderHasBeenCreated;
	}

	const std::string& url = video_config_["url"];
	if (url.empty()) {
		LOG(ERROR) << "url is empty";
		return ErrorCode::kCreateReaderFail;
	}

	if (NULL != av_options_) {
		av_dict_free(&av_options_);
		av_options_ = NULL;
	}

	const std::string& max_delay = video_config_["max_delay"];
	if (!max_delay.empty()) {
		av_dict_set(&av_options_, "max_delay", max_delay.c_str(), 0);
	}

	const std::string& rtsp_transport = video_config_["rtsp_transport"];
	if (!rtsp_transport.empty()) {
		av_dict_set(&av_options_, "rtsp_transport", rtsp_transport.c_str(), 0);
	}

	const std::string& stimeout = video_config_["stimeout"];
	if (!stimeout.empty()) {
		av_dict_set(&av_options_, "stimeout", stimeout.c_str(), 0);
	}

	const std::string& buffer_size = video_config_["buffer_size"];
	if (!buffer_size.empty()) {
		av_dict_set(&av_options_, "buffer_size", buffer_size.c_str(), 0);
	}

	if (NULL != av_format_context_) {
		avformat_close_input(&av_format_context_);
	}

	if (avformat_open_input(&av_format_context_, url.c_str(), NULL, &av_options_) < 0) {
		LOG(ERROR) << "open url fail! url:" << url;
		return ErrorCode::kCreateReaderFail;
	}

	if (avformat_find_stream_info(av_format_context_, NULL) < 0) {
		LOG(ERROR) <<"find stream info fail! url:" << url;
		return ErrorCode::kCreateReaderFail;
	}

	video_stream_index_ = -1;
	for (int i = 0; i < av_format_context_->nb_streams; ++i) {
		if (AVMEDIA_TYPE_VIDEO == av_format_context_->streams[i]->codecpar->codec_type) {
			video_stream_index_ = i;
			break;
		}
	}
	if (-1 == video_stream_index_) {
		LOG(ERROR) << "can not find a video stream in url! url:" << url;
		return ErrorCode::kCreateReaderFail;
	}

	const char* mp4toannexb_avbsf_name = nullptr;
	switch (av_format_context_->streams[video_stream_index_]->codecpar->codec_id) {
		case AV_CODEC_ID_H264:
			mp4toannexb_avbsf_name = "h264_mp4toannexb";
			break;
		case AV_CODEC_ID_HEVC:
			mp4toannexb_avbsf_name = "hevc_mp4toannexb";
			break;
		default:
			LOG(ERROR) << "unsupport codec type! codec id :" << av_format_context_->streams[video_stream_index_]->codecpar->codec_id;
			return ErrorCode::kCreateReaderFail;
	}

	if (NULL != mp4toannexb_avbsf_) {
		av_bsf_free(&mp4toannexb_avbsf_);
	}

	if (av_bsf_alloc(av_bsf_get_by_name(mp4toannexb_avbsf_name), &mp4toannexb_avbsf_) < 0) {
		LOG(WARNING) << "alloc av bsf fail";
	} else {
		avcodec_parameters_copy(mp4toannexb_avbsf_->par_in, av_format_context_->streams[video_stream_index_]->codecpar);
		av_bsf_init(mp4toannexb_avbsf_);
	}

	if (NULL != dump_extra_avbsf_) {
		av_bsf_free(&dump_extra_avbsf_);
	}

	if (av_bsf_alloc(av_bsf_get_by_name("dump_extra"), &dump_extra_avbsf_) < 0) {
		LOG(WARNING) << "alloc av bsf fail!";
	} else {
		avcodec_parameters_copy(dump_extra_avbsf_->par_in, 
				mp4toannexb_avbsf_ ? mp4toannexb_avbsf_->par_out : av_format_context_->streams[video_stream_index_]->codecpar);
		av_bsf_init(dump_extra_avbsf_);
	}

	return ErrorCode::kOk;
}

void FFmpegVideoReader::Release() {
	std::lock_guard<std::recursive_mutex> lock(mutex_);
	if (NULL != dump_extra_avbsf_) {
		av_bsf_free(&dump_extra_avbsf_);
		dump_extra_avbsf_ = NULL;
	}

	if (NULL != mp4toannexb_avbsf_) {
		av_bsf_free(&mp4toannexb_avbsf_);
		mp4toannexb_avbsf_ = NULL;
	}

	if (NULL != av_format_context_) {
		avformat_close_input(&av_format_context_);
		av_format_context_ = NULL;
	}

	if (NULL != av_options_) {
		av_dict_free(&av_options_);
		av_options_ = NULL;
	}
}

void FFmpegVideoReader::Close() {
	Release();
}

ErrorCode FFmpegVideoReader::ReceivePacket(Packet& packet) {
	std::lock_guard<std::recursive_mutex> lock(mutex_);
	if (NULL == av_format_context_) {
		if (ErrorCode::kOk != Open()) {
			return ErrorCode::kCreateReaderFail;
		}
	}

	AVPacket av_packet;
	do {
		av_init_packet(&av_packet);
		if (av_read_frame(av_format_context_, &av_packet) < 0) {
			Close();
			LOG(ERROR) << "read frame fail";
			return ErrorCode::kReadFail;
		}

		if (av_packet.stream_index != video_stream_index_) {
			av_packet_unref(&av_packet);
			continue;
		} else {
			break;
		}
	} while (true);

	if (NULL != mp4toannexb_avbsf_) {
		if (av_bsf_send_packet(mp4toannexb_avbsf_, &av_packet) < 0) {
			LOG(ERROR) << "send packet to bsf fail";
			av_packet_unref(&av_packet);
			return ErrorCode::kReadFail;
		}

		if (av_bsf_receive_packet(mp4toannexb_avbsf_, &av_packet) < 0) {
			LOG(ERROR) << "receive packet from bsf fail";
			return ErrorCode::kReadFail;
		}
	}

	if (NULL != dump_extra_avbsf_) {
		if (av_bsf_send_packet(dump_extra_avbsf_, &av_packet) < 0) {
			av_packet_unref(&av_packet);
			return ErrorCode::kReadFail;
		}

		if (av_bsf_receive_packet(dump_extra_avbsf_, &av_packet) < 0) {
			LOG(ERROR) << "receive packet from bsf fail";
			return ErrorCode::kReadFail;
		}
	}

	packet.key_frame = av_packet.flags == AV_PKT_FLAG_KEY;
	packet.data_size = av_packet.size;
	packet.data = std::shared_ptr<std::uint8_t>(new std::uint8_t[packet.data_size], [](std::uint8_t* data) {delete[] data;});
	std::memcpy(packet.data.get(), av_packet.data, packet.data_size);
	av_packet_unref(&av_packet);

	AVCodecParameters* codecpar = av_format_context_->streams[video_stream_index_]->codecpar;
	switch (codecpar->codec_id) {
		case AV_CODEC_ID_H264:
			packet.codec_type = CodecType::kH264;
			break;
		case AV_CODEC_ID_HEVC:
			packet.codec_type = CodecType::kHevc;
			break;
		default:
			packet.codec_type = CodecType::kUnknown;
			break;
	}

	packet.width = codecpar->width;
	packet.height = codecpar->height;

	return ErrorCode::kOk;
}

}//namespace ffmpeg
}//namespace video
