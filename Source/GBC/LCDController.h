#pragma once

#include "Core/Pointers.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace GBC
{

constexpr size_t kScreenWidth = 160;
constexpr size_t kScreenHeight = 144;

// 15 bits per pixel
struct Pixel
{
   uint8_t r;
   uint8_t g;
   uint8_t b;

   Pixel(uint8_t red = 0x00, uint8_t green = 0x00, uint8_t blue = 0x00)
      : r(red)
      , g(green)
      , b(blue)
   {
   }
};

using Framebuffer = std::array<Pixel, kScreenWidth * kScreenHeight>;

class DoubleBufferedFramebuffer
{
public:
   DoubleBufferedFramebuffer()
      : writeIndex(0)
   {
      for (size_t i = 0; i < buffers.size(); ++i)
      {
         buffers[i] = UPtr<Framebuffer>(new Framebuffer{});
      }
   }

   Framebuffer& writeBuffer() const
   {
      return *buffers[writeIndex];
   }

   const Framebuffer& readBuffer() const
   {
      return *buffers[!writeIndex];
   }

   void flip()
   {
      writeIndex = (writeIndex + 1) % buffers.size();
   }

private:
   std::array<UPtr<Framebuffer>, 2> buffers;
   size_t writeIndex;
};

class LCDController
{
public:
   LCDController(class Memory& memory);

   void tick(uint64_t totalCycles, bool cpuStopped);

   const Framebuffer& getFramebuffer() const
   {
      return framebuffers.readBuffer();
   }

private:
   void scan(Framebuffer& framebuffer, uint8_t line, const std::array<Pixel, 4>& colors);
   void scanBackgroundOrWindow(Framebuffer& framebuffer, uint8_t line, const std::array<Pixel, 4>& colors,
                               bool isWindow);
   void scanSprites(Framebuffer& framebuffer, uint8_t line);
   uint8_t fetchPaletteIndex(uint8_t tileNum, uint8_t row, uint8_t col, bool signedTileOffset, bool flipX) const;

   class Memory& mem;
   DoubleBufferedFramebuffer framebuffers;
   std::array<uint8_t, kScreenWidth * kScreenHeight> bgPaletteIndices;
};

} // namespace GBC
