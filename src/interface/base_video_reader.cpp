#include "interface/base_video_reader.h"
namespace video {

BaseVideoReader::BaseVideoReader() {
}
BaseVideoReader::~BaseVideoReader() {
}

void BaseVideoReader::SetVideoConfig(const VideoConfig& video_config) {
	std::lock_guard<std::mutex> lock(mutex_);
	video_config_ = video_config;
}

void BaseVideoReader::SetEventCallback(EventCallback callback) {
	std::lock_guard<std::mutex> lock(mutex_);
	event_callback_ = callback;
}

}//namespace video
