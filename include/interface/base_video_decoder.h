#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <queue>

#include "error_code.h"
#include "type_define.h"
#include "frame.h"
#include "packet.h"

namespace video {

class BaseVideoDecoder {
public:
	typedef std::function<void(const Frame&)> FrameCallback;

	BaseVideoDecoder();
	virtual ~BaseVideoDecoder();

	DELETE_COPY(BaseVideoDecoder)

	virtual void SetVideoConfig(VideoConfig& video_config);
	void SetFrameCallback(FrameCallback callback);
	virtual ErrorCode ReceiveFrame(std::chrono::milliseconds timeout, Frame& frame);
	virtual void SendFrame(const Frame& frame);
	virtual ErrorCode SendPacket(const Packet& packet) = 0;

	static std::size_t kMaxFrameQueueSize;

protected:
	int device_id_;
	int channel_id_;
	FormatType format_type_;
	FrameCallback frame_callback_;
	std::size_t pump_frame_rate_;
	std::size_t max_frame_queue_size_;
	std::queue<Frame> frame_queue_;
	std::mutex frame_queue_mutex_;
	std::mutex mutex_;
	std::condition_variable frame_queue_condition_variable_;
};
}//namespace video
