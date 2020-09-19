#pragma once
#include <functional>
#include <map>
#include "frame.h"
#include "event.h"


#define DELETE_COPY(name)					\
	name(const name&) = delete;					\
	name(name&&) = delete;						\
	name& operator = (const name&) = delete;	\
	name& operator = (name&&) = delete;			
namespace video {

typedef std::map<std::string, std::string> VideoConfig;

}//namespace video
