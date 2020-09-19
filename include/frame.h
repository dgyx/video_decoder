#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

namespace video {

enum class FormatType {
    kUnknown, 
    kNv12, 
    kNv21, 
    kBgrPacket, 
    kBgrPlanar, 
};

struct Frame {
    Frame()
        : format_type(FormatType::kUnknown),
          width(0),
          height(0), 
          step(0), 
          data_size(0) {
    }

    ~Frame() {
    }

    FormatType format_type;
    std::size_t width;
    std::size_t height;
    std::size_t step;
    std::size_t data_size;
    std::shared_ptr<std::uint8_t> data; 
};

}

