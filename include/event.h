#pragma once
#include <string>

#include "error_code.h"

namespace video {

struct Event {
    ErrorCode error_code;
    std::string message;
};

}

