#include "interface/base_video_decode.h"

namespace video {

std::size_t BaseVideoDecoder::kMaxFrameQueueSize = 25;

BaseVideoDecoder::BaseVideoDecoder() 
	: device_id_(-1), 
	  channel_id(-1),
	  format_type_(FormatType::kBgrPacket),
	  pump_frame_rate_(0),
	  max_frame_queue_size_(kMaxFrameQueueSize) {
}

BaseVideoDecoder::~BaseVideoDecoder() {
}

void BaseVideoDecoder::SetVideoConfig(VideoConfig& video_config) {
	try {
		std::lock_guard<std::mutex> lock(mutex_);
		if (video_config.end() != video_config.find("channel_id")) {
			channel_id_ = std::atoi(video_config["channel_id"]);
		}
		if (video_config.end() != video_config.find("device_id_")) {
			device_id_ = std::atoi(video_config["device_id_"];
		}
		if (video_config.end() != video_config.find("format_type")) {
			format_type_ = std::atoi(video_config["format_type"];
		}
		if (video_config.end() != video_config.find("pump_frame_rate")) {
			pump_frame_rate_ = static_cast<std::size_t>(std::atol(video_config["pump_frame_rate"]);
		}
		if (video_config.end() != video_config.find("max_frame_queue_size")) {
			max_frame_queue_size_ = static_cast<std::size_t>(std::atol(video_config["max_frame_queue_size"]);
		}
	} catch (std::exception& e) {
		LOG(AWRNING) << "[video decoder] video config error!" << e.what();
	}
}

void BaseVideoDecoder::SetFrameCallback(FrameCallback callback) {
	std::lock_guard<std::mutex> lock(mutex_);
	frame_callback_ = callback;
}

ErrorCode BaseVideoDecoder::ReceiveFrame(std::chrono::milliseconds timeout, Frame& frame) {
	std::unique_lock<std::mutex> lock(frame_queue_mutex_);
	auto& frame_queue = frame_queue_;
	if (!frame_queue_condition_variable_.wait_for(lock, timeout, [&frame_queue]() { return !frame_queue.empty(); })) {
		LOG(WARNING) << "receive channel frame timeout" << channel_id_;
		return ErrorCode::kTimeout;
	}

	frame = frame_queue.front();
	frame_queue_.pop();
	return ErrorCode::kOk;
}

ErrorCode BaseVideoDecoder::SendFrame(const Frame& frame) {
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (frame_callback_) {
			frame_callback_(frame);
			return;
		}
	}

	std::lock_guard<std::mutex> lock(frame_queue_mutex_);
	while (frame_queue_.size() > max_frame_queue_size_) {
		LOG(WARNING) << "frame queue is full, discard oldest frame! max frame queue size:" << max_frame_queue_size_;
		frame_queue_.pop();
	}
	frame_queue_.push(frame);
	frame_queue_condition_variable_.notify_one();
}

}//namespace video
