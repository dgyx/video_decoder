#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

namespace video {

enum class CodecType {
    kUnknown, 
    kH264, 
    kH265, 
    kHevc = kH265, 
};

struct Packet {
    Packet()
        : codec_type(CodecType::kUnknown), 
          key_frame(false), 
          width(0), 
          height(0), 
          data_size(0) {
    }

    ~Packet() {
    }

    CodecType codec_type;
    bool key_frame;
    std::size_t width;
    std::size_t height;
    std::size_t data_size;
    std::shared_ptr<std::uint8_t> data; 
};

}
