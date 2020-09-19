#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "error_code.h"
#include "event.h"
#include "packet.h"
#include "type_define.h"

namespace video {

class BaseVideoReader {
public:
	typedef std::function<void(const Event&)> EventCallback;

	BaseVideoReader();
	virtual ~BaseVideoReader();

	DELETE_COPY(BaseVideoReader)

	virtual ErrorCode Open() = 0;
	virtual void Close() = 0;
	virtual void SetVideoConfig(const VideoConfig& video_config);
	virtual void SetEventCallback(EventCallback event_callback);
	virtual ErrorCode ReceivePacket(Packet& packet) = 0;

protected:
	VideoConfig video_config_;
	EventCallback event_callback_;
	std::recursive_mutex mutex_;
};


}//namespace video
