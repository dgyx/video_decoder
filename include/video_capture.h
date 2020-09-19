#pragma once
#include <atomic>
#include <thread>

#include "error_code.h"
#include "event.h"
#include "frame.h"
#include "interface/base_video_reader.h"
#include "interface/base_video_decoder.h"
#include "type_define.h"

namespace video {
class VideoCapture {
public:
	typedef std::function<std::shared_ptr<BaseVideoReader>()> VideoReaderCreator;
	typedef std::function<std::shared_ptr<BaseVideoDecoder>()> VideoDecoderCreator;
	typedef BaseVideoReader::EventCallback EventCallback;
	typedef BaseVideoDecoder::FrameCallback FrameCallback;

	DELETE_COPY(VideoCapture)
	
	VideoCapture();
	virtual ~VideoCapture();
	
	void SetChannelVideoConfig(int channel_id, const VideoConfig& video_config);
	void SetChannelEventCallback(int channel_id, EventCallback callback);
	void SetChannelFrameCallback(int channel_id, FrameCallback callback);
	void SetChannelVideoReaderCreator(std::function<std::shared_ptr<BaseVideoReader>()> video_reader_creator);
	void SetChannelVideoDecoderCreator(std::function<std::shared_ptr<BaseVideoDecoder>()> video_decoder_creator);
	
	ErrorCode StartAllChannel();
	void StopAllChannel();
	
	ErrorCode StartChannel(int channel_id);
	ErrorCode StopChannel(int channel_id);
	void EraseChannel(int channel_id);
	
	ErrorCode ReceiveChannelFrame(int channel_id, std::chrono::milliseconds timeout, Frame& frame);

protected:
	class Channel {
	public:
		Channel();
		virtual ~Channel();

		DELETE_COPY(Channel)

		void set_channel_id(int channel_id) {
			channel_id_ = channel_id;
		}

		void set_video_config(const VideoConfig& video_config) {
			std::lock_guard<std::mutex> lock(mutex_);
			video_config_ = video_config;
		}

		void set_event_callback(EventCallback callback) {
			std::lock_guard<std::mutex> lock(mutex_);
			event_callback_ = callback;
		}

		void set_receive_frame_callback(FrameCallback callback) {
			std::lock_guard<std::mutex> lock(mutex_);
			receive_frame_callback_ = callback;
		}

		ErrorCode Start(VideoReaderCreator video_reader_creator, VideoDecoderCreator video_decoder_creator);
		ErrorCode Stop();

		ErrorCode ReceiveFrame(std::chrono::milliseconds timeout, Frame& frame);
	private:
		void ChannelVideoDecode();

	private:
		int channel_id_;
		VideoConfig video_config_;
		EventCallback event_callback_;
		int device_id_;
		FormatType format_type_;
		std::size_t pump_frame_rate_;
		std::size_t max_frame_queue_size_;
		FrameCallback receive_frame_callback_;
		std::mutex mutex_;

		std::atomic_bool stop_flag_;
		std::shared_ptr<BaseVideoReader> video_reader_;
		std::shared_ptr<BaseVideoDecoder> video_decoder_;
		std::shared_ptr<std::thread> thread_;
	};

	std::shared_ptr<Channel> FindChannelWithCreate(int channel_id);
	std::shared_ptr<Channel> FindChannel(int channel_id) const;
	void DelChannel(int channel_id);

	VideoReaderCreator video_reader_creator_;
	VideoDecoderCreator video_decoder_creator_;

	std::map<int, std::shared_ptr<Channel> > channel_map_;
	mutable std::mutex channel_map_mutex_;
};



}//namespace video
