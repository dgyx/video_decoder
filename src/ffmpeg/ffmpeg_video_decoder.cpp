#include "ffmpeg/ffmpeg_video_decoder.h"


namespace video {
namespace ffmpeg {

FFmpegVideoDecoder::FFmpegVideoDecoder() : av_codec_context_(NULL), current_frame_id_(0) {
	avcodec_register_all();
}

FFmpegVideoDecoder::~FFmpegVideoDecoder() {
	if (NULL != av_codec_context_) {
		avcodec_free_context(&av_codec_context_);
		av_codec_context_ = NULL;
	}
	if (NULL != sws_context_) {
		sws_freeContext(sws_context);
		sws_context = NULL;
	}
}

ErrorCode FFmpegVideoDecoder::SendPacket(const Packet& packet) {
	if (0 == packet.data_size) {
		avcodec_free_context(&av_codec_context_);
		av_codec_context_ = NULL;
		sws_freeContext(sws_context_);
		sws_context_ = NULL;
		return ErrorCode::kOk;
	}

	AVCodecID av_codec_id = AV_CODEC_ID_NONE;
	const char * av_codec_name = NULL;
	switch (packet.codec_type) {
		case CodecType::kH264:
			av_codec_id = AV_CODEC_ID_H264;
			av_codec_name = -1 == device_id_ ? "h264" : "h264_cuvid";
			break;
		case CodecType::kHevc:
			av_codec_id = AV_CODEC_ID_HEVC;
			av_codec_name = -1 == device_id_ ? "hevc" : "hevc_cuvid";
			break;
		default:
			LOG(ERROR) << "unsupport codec type! codec type:" << static_cast<int>(packet.codec_type);
			return ErrorCode::kDecodeFail;
	}

	if (NULL != av_codec_context_ && av_codec_id != av_codec_context_->codec_id) {
		avcodec_free_context(&av_codec_context_);
	}

	if (NULL == av_codec_context_) {
		AVCodec* av_codec = avcodec_find_decoder_by_name(av_codec_name);
		if (NULL == av_codec) {
			LOG(ERROR) << "find decoder by name failed codec name:" << av_codec_name;
			return ErrorCode::kDecodeFail;
		}

		av_codec_context_ = avcodec_alloc_context3(av_codec);
		if (NULL == av_codec_context_) {
			LOG(ERROR) << "alloc codec context failed";
			return ErrorCode::kDecodeFail;
		}

		AVDictionary* av_options = NULL;
		av_dict_set(&av_options, "gpu", std::to_string(device_id_).c_str(), 0);
		int result = avcodec_open2(av_codec_context_, av_codec, &av_options);
		av_dict_free(&av_options);
		if (0 != result) {
			avcodec_free_context(&av_codec_context_);
			LOG(ERROR) << "open codec failed";
			return ErrorCode::kDecodeFail;
		}
	}

	int send_result = 0;
	int receive_result = 0;
	std::vector<AVFrame*> av_frame_list;

	do {
		AVPacket av_packet;
		av_init_packet(&av_packet);
		av_packet.data = static_cast<std::uint8_t*>(packet.data.get());
		av_packet.size = packet.data_size;

		send_result = avcodec_send_packet(av_codec_context_, &av_packet);
		if (0 != send_result && AVERROR(EAGAIN) != send_result) {
			LOG(ERROR) << "send codec packet failed! result:" << send_result;
			break;
		}

		do {
			AVFrame* av_frame = av_frame_alloc();
			if (NULL == av_frame) {
				LOG(ERROR) << "alloc frame failed";
				break;
			}

			receive_result = avcodec_receive_frame(av_codec_context_, av_frame);
			if (0 != receive_result) {
				av_frame_free(&av_frame);
				break;
			}

			av_frame_list.push_back(av_frame);
		} while (true);
		if (0 != receive_result && AVERROR(EAGAIN) != receive_result) {
			LOG(ERROR) << "receive codec frame failed! receive result:" << receive_result;
			break;
		}
	} while (AVERROR(EAGAIN) == send_result);

	if (0 != send_result
			&& AVERROR(EAGAIN) != send_result
			&& 0 != receive_result
			&& AVERROR(EAGAIN) != receive_result) {
		return ErrorCode::kDecodeFail;
	}

	for (auto& av_frame : av_frame_list) {
		if (0 != pump_frame_rate_ && 0 != current_frame_id++ % pump_frame_rate_) {
			continue;
		}

		AVCodecContext* av_codec_context = av_codec_context_;
		if (NULL == sws_context_) {
			sws_context_ = sws_getContext(av_codec_context_->width, av_codec_context_->height, av_codec_context_->pix_fmt,
					av_codec_context_->width, av_codec_context_->height, AV_PIX_FMT_BGR24, 0, NULL, NULL, NULL);
			if (NULL == sws_context_) {
				av_frame_free(&av_frame);
				LOG(ERROR) << "get sws context failed!";
				continue;
			}
		}

		Frame frame;
		frame.format_type = FormatType::kBgrPacket;
		frame.width = av_frame->width;
		frame.height = av_frame->height;
		frame.step = 3 * av_frame->width;
		frame.data_size = 3 * frame.width * frame.height;
		frame.data = std::shared_ptr<std:uint8_t>(new std::uint8_t[frame.data_size], [](std::uint8_t* data) { delete [] data; });

		std::uint8_t* data[1] = { frame.data.get() };
		int linesize[1] = { static_cast<int>(frame.step) };
		sws_scale(sws_context_, av_frame->data, av_frame->linesize, 0, av_frame->height, data, linesize);

		av_frame_free(&av_frame);
		SendFrame(frame);
	}
	return ErrorCode::kOk;
}

}//namespace ffmpeg
}//namespace video
