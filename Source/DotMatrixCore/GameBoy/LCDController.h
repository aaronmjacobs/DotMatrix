#pragma once

#include "DotMatrixCore/Core/Enum.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace DotMatrix
{

class GameBoy;

constexpr size_t kScreenWidth = 160;
constexpr size_t kScreenHeight = 144;

using Framebuffer = std::array<uint8_t, kScreenWidth * kScreenHeight>;

class DoubleBufferedFramebuffer
{
public:
   DoubleBufferedFramebuffer()
   {
      for (std::size_t i = 0; i < buffers.size(); ++i)
      {
         buffers[i] = std::make_unique<Framebuffer>(Framebuffer{});
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
   std::array<std::unique_ptr<Framebuffer>, 2> buffers;
   std::size_t writeIndex = 0;
};

class LCDController
{
public:
   LCDController(GameBoy& gb);

   void machineCycle()
   {
      updateDMA();
      updateMode();
   }

   void onCPUStopped();

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   const Framebuffer& getFramebuffer() const
   {
      return framebuffers.readBuffer();
   }

private:
   enum class Mode : uint8_t
   {
      HBlank = 0,
      VBlank = 1,
      SearchOAM = 2,
      DataTransfer = 3
   };

   struct ControlRegister
   {
      bool lcdDisplayEnabled = false;
      bool windowUseUpperTileMap = false;
      bool windowDisplayEnabled = false;
      bool bgAndWindowUseUnsignedTileData = false;
      bool bgUseUpperTileMap = false;
      bool useLargeSpriteSize = false;
      bool spriteDisplayEnabled = false;
      bool bgWindowDisplayEnabled = false;

      uint8_t read() const;
      void write(uint8_t value);
   };

   struct StatusRegister
   {
      bool coincidenceInterrupt = false;
      bool oamInterrupt = false;
      bool vBlankInterrupt = false;
      bool hBlankInterrupt = false;
      bool coincidenceFlag = false;
      Mode mode = Mode::VBlank;

      uint8_t read() const;
      void write(uint8_t value);
   };

   struct SpriteAttributes
   {
      uint8_t yPos = 0;
      uint8_t xPos = 0;
      uint8_t tileNum = 0;
      uint8_t flags = 0;
   };

   struct TileLine
   {
      uint8_t firstByte = 0x00;
      uint8_t secondByte = 0x00;
   };

   void updateDMA();
   void updateMode();
   void updateLYC();
   void setMode(Mode newMode);

   void scan(Framebuffer& framebuffer, uint8_t line, const std::array<uint8_t, 4>& paletteColors);
   template<bool isWindow>
   void scanBackgroundOrWindow(Framebuffer& framebuffer, uint8_t line, const std::array<uint8_t, 4>& paletteColors);
   void scanSprites(Framebuffer& framebuffer, uint8_t line);

   TileLine fetchTileLine(uint8_t tileNum, uint8_t line, bool signedTileOffset) const;

   bool isSpriteAttributeTableAccessible() const
   {
      return dmaIndex == 0x00;
   }

   GameBoy& gameBoy;

   uint32_t modeCyclesRemaining = 0;

   bool dmaRequested = false;
   bool dmaPending = false;
   bool dmaInProgress = false;
   uint8_t dmaIndex = 0;
   uint16_t dmaSource = 0;

   ControlRegister controlRegister;
   StatusRegister statusRegister;

   uint8_t scy = 0;
   uint8_t scx = 0;
   uint8_t ly = 144;
   uint8_t lyc = 0;
   uint8_t dma = 0;
   uint8_t bgp = 0;
   uint8_t obp0 = 0;
   uint8_t obp1 = 0;
   uint8_t wy = 0;
   uint8_t wx = 0;

   std::array<uint8_t, 0x2000> vram = {};
   union
   {
      std::array<SpriteAttributes, 0x0040> spriteAttributes;
      std::array<uint8_t, 0x0100> oam = {};
   };

   DoubleBufferedFramebuffer framebuffers;
   std::array<uint8_t, kScreenWidth * kScreenHeight> bgPaletteIndices = {};
};

} // namespace DotMatrix
