#include <glog/logging.h>

#include "video_capture.h"

namespace video {

VideoCapture::VideoCapture() {
}

VideoCapture::~VideoCapture() {
}

void VideoCapture::SetChannelVideoConfig(int channel_id, const VideoConfig& video_config) {
	FindChannelWithCreate(channel_id)->set_video_config(video_config);
}

void VideoCapture::SetChannelEventCallback(int channel_id, EventCallback callback) {
	FindChannelWithCreate(channel_id)->set_event_callback(callback);
}

void VideoCapture::SetChannelFrameCallback(int channel_id, FrameCallback callback) {
	FindChannelWithCreate(channel_id)->set_receive_frame_callback(callback);
}

ErrorCode VideoCapture::StartAllChannel() {
	std::lock_guard<std::mutex> lock(channel_map_mutex_);
	bool start_succ = false;
	for (auto& channel : channel_map_) {
		ErrorCode err = channel.second->Start(video_reader_creator_, video_decoder_creator_);
		if (ErrorCode::kOk != err) {
			LOG(WARNING) << "channel id:" << channel.first << " start failed! ErrorCode:" << static_cast<int>(err);
		} else {
			start_succ = true;
		}
	}
	if (start_succ) {
		return ErrorCode::kOk;
	}
	return ErrorCode::kChannelStartFail;
}

void VideoCapture::StopAllChannel() {
	std::lock_guard<std::mutex> lock(channel_map_mutex_);
	for (auto& channel : channel_map_) {
		channel.second->Stop();
	}
}

ErrorCode VideoCapture::StartChannel(int channel_id) {
	auto channel = FindChannel(channel_id);
	if (channel) {
		return channel->Start(video_reader_creator_, video_decoder_creator_);
	} else {
		LOG(ERROR) << "can not find channel, channel id: " << channel_id;
		return ErrorCode::kChannelNotFound;
	}
}

ErrorCode VideoCapture::StopChannel(int channel_id) {
	auto channel = FindChannel(channel_id);
	if (channel) {
		return channel->Stop();
	} else {
		LOG(ERROR) << "can not find channel, channel id: " << channel_id;
		return ErrorCode::kChannelNotFound;
	}
}

ErrorCode VideoCapture::ReceiveChannelFrame(int channel_id, std::chrono::milliseconds timeout, Frame& frame) {
	auto channel = FindChannel(channel_id);
	if (channel) {
		return channel->ReceiveFrame(timeout, frame);
	} else {
		LOG(ERROR) << "can not find channel, channel id: " << channel_id;
		return ErrorCode::kChannelNotFound;
	}
}

VideoCapture::Channel::Channel()
	: channel_id_(-1),
	  device_id_(-1),
	  format_type_(FormatType::kBgrPacket),
	  pump_frame_rate_(0),
	  max_frame_queue_size_(-1),
	  stop_flag_(true),
	  video_reader_(nullptr),
	  video_decoder_(nullptr) {
}

VideoCapture::Channel::~Channel() {
	Stop();
}

ErrorCode VideoCapture::Channel::Start(VideoReaderCreator video_reader_creator, VideoDecoderCreator video_decoder_creator) {
	bool stop_flag = true;
	if (!stop_flag_.compare_exchange_strong(stop_flag, false) || (thread_ && thread_->joinable())) {
		return ErrorCode::kChannelHasBeenRun;
	}

	if (!video_reader_) {
		if (!video_reader_creator) {
			LOG(ERROR) << "can not dinf video reader creator";
			return ErrorCode::kReaderCreatorNotFound;
		} else {
			video_reader_ = video_reader_creator();
		}
	}

	video_reader_->SetVideoConfig(video_config_);
	video_reader_->SetEventCallback(event_callback_);

	if (!video_decoder_) {
		if (!video_decoder_creator) {
			LOG(ERROR) << "can not find video decoder creator";
			return ErrorCode::kDecoderCreatorNotFound;
		} else {
			video_decoder_ = video_decoder_creator();
		}
	}

	video_decoder_->SetVideoConfig(video_config_);
	video_decoder_->SetFrameCallback(receive_frame_callback_);

	thread_ = std::make_shared<std::thread>(std::bind(&Channel::ChannelVideoDecode, this));
	return ErrorCode::kOk;
}

ErrorCode VideoCapture::Channel::Stop() {
	bool stop_flag = false;
	ErrorCode err = ErrorCode::kOk;
	if (!stop_flag_.compare_exchange_strong(stop_flag, true)) {
		err = ErrorCode::kChannelIsNotRun;
	}

	if (thread_ && thread_->joinable()) {
		thread_->join();
	}
	return err;
}

void VideoCapture::Channel::ChannelVideoDecode() {
	while (!stop_flag_) {
		Packet packet;
		if (!video_reader_) {
			LOG(ERROR) << "video reader is null, channel id:" << channel_id_;
			stop_flag_ = true;
			break;
		}

		if (ErrorCode::kOk != video_reader_->ReceivePacket(packet)) {
			LOG(ERROR) << "receive frame from reader fail, channel_id:" << channel_id_;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		if (!video_decoder_) {
			LOG(ERROR) << "video decoder is null, channel id:" << channel_id_;
			stop_flag_ = true;
			break;
		}

		if (ErrorCode::kOk != video_decoder_->SendPacket(packet)) {
			LOG(ERROR) << "send frame to decoder fail, channel id:" << channel_id_;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
	}

	video_reader_->Close();

	if (video_decoder_) {
		if (ErrorCode::kOk != video_decoder_->SendPacket(Packet())) {
			LOG(ERROR) << "send end frame fail, channel_id:" << channel_id_;
		}
	}
}

ErrorCode VideoCapture::Channel::ReceiveFrame(std::chrono::milliseconds timeout, Frame& frame) {
	if (stop_flag_) {
		return ErrorCode::kChannelIsNotRun;
	}

	if (ErrorCode::kOk != video_decoder_->ReceiveFrame(timeout, frame)) {
		return ErrorCode::kTimeout;
	}

	return ErrorCode::kOk;
}

std::shared_ptr<VideoCapture::Channel> VideoCapture::FindChannelWithCreate(int channel_id) {
	std::lock_guard<std::mutex> lock(channel_map_mutex_);
	auto find_it = channel_map_.find(channel_id);
	if (channel_map_.end() == find_it) {
		auto channel = std::make_shared<Channel>();
		channel->set_channel_id(channel_id);
		channel_map_[channel_id] = channel;
		return channel;
	} else {
		return find_it->second;
	}
}

std::shared_ptr<VideoCapture::Channel> VideoCapture::FindChannel(int channel_id) const {
	std::lock_guard<std::mutex> lock(channel_map_mutex_);
	auto find_it = channel_map_.find(channel_id);
	if (channel_map_.end() == find_it) {
		return std::shared_ptr<Channel>();
	} else {
		return find_it->second;
	}
}

void VideoCapture::EraseChannel(int channel_id) {
	std::lock_guard<std::mutex> lock(channel_map_mutex_);
	channel_map_.erase(channel_id);
}

}//namespace video
