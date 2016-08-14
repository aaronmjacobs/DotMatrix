#ifndef GBC_LCD_CONTROLLER_H
#define GBC_LCD_CONTROLLER_H

#include "Pointers.h"

#include <cstddef>
#include <cstdint>

namespace GBC {

constexpr size_t kScreenWidth = 160;
constexpr size_t kScreenHeight = 144;

// 15 bits per pixel
struct Pixel {
   uint8_t r;
   uint8_t g;
   uint8_t b;

   Pixel()
      : r(0x00), g(0x00), b(0x00) {
   }

   Pixel(uint8_t red, uint8_t green, uint8_t blue)
      : r(red), g(green), b(blue) {
   }
};

using Framebuffer = std::array<Pixel, kScreenWidth * kScreenHeight>;

class DoubleBufferedFramebuffer {
public:
   DoubleBufferedFramebuffer()
      : writeIndex(0) {
      for (size_t i = 0; i < buffers.size(); ++i) {
         buffers[i] = UPtr<Framebuffer>(new Framebuffer{});
      }
   }

   Framebuffer& writeBuffer() const {
      return *buffers[writeIndex];
   }

   const Framebuffer& readBuffer() const {
      return *buffers[!writeIndex];
   }

   void flip() {
      writeIndex = (writeIndex + 1) % buffers.size();
   }

private:
   std::array<UPtr<Framebuffer>, 2> buffers;
   size_t writeIndex;
};

class LCDController {
public:
   LCDController(class Memory& memory);

   void tick(uint64_t cycles);

   const Framebuffer& getFramebuffer() const {
      return framebuffers.readBuffer();
   }

private:
   void scan(Framebuffer& framebuffer, const std::array<Pixel, 4>& colors, uint8_t line);
   void scanBackgroundOrWindow(Framebuffer& framebuffer, const std::array<Pixel, 4>& colors, uint8_t line, bool isWindow);
   void scanBackground(Framebuffer& framebuffer, const std::array<Pixel, 4>& colors, uint8_t line);
   void scanWindow(Framebuffer& framebuffer, const std::array<Pixel, 4>& colors, uint8_t line);
   void scanSprites(Framebuffer& framebuffer, const std::array<Pixel, 4>& colors, uint8_t line);

   class Memory& mem;
   DoubleBufferedFramebuffer framebuffers;
};

} // namespace GBC

#endif
