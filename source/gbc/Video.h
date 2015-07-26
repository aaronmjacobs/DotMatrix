#ifndef VIDEO_H
#define VIDEO_H

#include <cstddef>
#include <cstdint>

namespace GBC {

const size_t SCREEN_WIDTH = 160;
const size_t SCREEN_HEIGHT = 144;

// 15 bits per pixel
struct Pixel {
   uint8_t r;
   uint8_t g;
   uint8_t b;
};

} // namespace GBC

#endif
