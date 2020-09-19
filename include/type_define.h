#pragma once
#include <functional>
#include "frame.h"
#include "event.h"
typedef std::map<std::string, std::string> VideoConfig;


#define DELETE_COPY(name)					\
	name(const name&) = delete;					\
	name(name&&) = delete;						\
	name& operator = (const name&) = delete;	\
	name& operator = (name&&) = delete;			
