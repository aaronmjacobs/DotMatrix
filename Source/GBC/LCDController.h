#pragma once

#include "Core/Pointers.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace GBC
{

class GameBoy;

constexpr size_t kScreenWidth = 160;
constexpr size_t kScreenHeight = 144;

namespace Mode
{

enum Enum : uint8_t
{
   HBlank = 0,
   VBlank = 1,
   SearchOAM = 2,
   DataTransfer = 3
};

} // namespace Mode

using Framebuffer = std::array<uint8_t, kScreenWidth * kScreenHeight>;

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
   LCDController(GameBoy& gb);

   void machineCycle();
   void onCPUStopped();

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   const Framebuffer& getFramebuffer() const
   {
      return framebuffers.readBuffer();
   }

private:
   struct SpriteAttributes
   {
      uint8_t yPos;
      uint8_t xPos;
      uint8_t tileNum;
      uint8_t flags;
   };

   void updateDMA();
   void updateMode();
   void updateLYC();
   void setMode(Mode::Enum newMode);

   void scan(Framebuffer& framebuffer, uint8_t line, const std::array<uint8_t, 4>& paletteColors);
   template<bool isWindow>
   void scanBackgroundOrWindow(Framebuffer& framebuffer, uint8_t line, const std::array<uint8_t, 4>& paletteColors);
   void scanSprites(Framebuffer& framebuffer, uint8_t line);
   uint8_t fetchPaletteIndex(uint8_t tileNum, uint8_t row, uint8_t col, bool signedTileOffset, bool flipX) const;

   bool isSpriteAttributeTableAccessible() const
   {
      return dmaIndex == 0x00;
   }

   GameBoy& gameBoy;

   uint32_t modeCyclesRemaining;

   bool dmaRequested;
   bool dmaPending;
   bool dmaInProgress;
   uint8_t dmaIndex;
   uint16_t dmaSource;

   uint8_t lcdc;
   uint8_t stat;
   uint8_t scy;
   uint8_t scx;
   uint8_t ly;
   uint8_t lyc;
   uint8_t dma;
   uint8_t bgp;
   uint8_t obp0;
   uint8_t obp1;
   uint8_t wy;
   uint8_t wx;

   std::array<uint8_t, 0x2000> vram;
   union
   {
      std::array<SpriteAttributes, 0x0040> spriteAttributes;
      std::array<uint8_t, 0x0100> oam;
   };

   DoubleBufferedFramebuffer framebuffers;
   std::array<uint8_t, kScreenWidth * kScreenHeight> bgPaletteIndices;
};

} // namespace GBC
