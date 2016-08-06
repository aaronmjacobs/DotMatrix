#include "Log.h"

#include "gbc/CPU.h"
#include "gbc/LCDController.h"
#include "gbc/Memory.h"

#include <array>

namespace GBC {

namespace {

namespace Mode {

enum Enum : uint8_t {
   kHBlank = 0,
   kVBlank = 1,
   kSearchOAM = 2,
   kDataTransfer = 3
};

} // namespace Mode

namespace LCDC {

enum Enum : uint8_t {
   kDisplayEnable = 1 << 7,
   kWindowTileMapDisplaySelect = 1 << 6,
   kWindowDisplayEnable = 1 << 5,
   kBGAndWindowTileDataSelect = 1 << 4,
   kBGTileMapDisplaySelect = 1 << 3,
   kObjSpriteSize = 1 << 2,
   kObjSpriteDisplayEnable = 1 << 1,
   kBGDisplay = 1 << 0
};

} // namespace LCDC

namespace STAT {

enum Enum : uint8_t {
   kLycLyCoincidence = 1 << 6,
   kMode2OAMInterrupt = 1 << 5,
   kMode1VBlankInterrupt = 1 << 4,
   kMode0HBlankInterrupt = 1 << 3,
   kCoincidenceFlag = 1 << 2,
   kModeFlag = (1 << 1) | (1 << 0)
};

} // namespace STAT

namespace Color {

enum Enum : uint8_t {
   kWhite = 0,
   kLightGray = 1,
   kDarkGray = 2,
   kBlack = 3
};

} // namespace Color

static const uint32_t kSearchOAMCycles = 80;
static const uint32_t kDataTransferCycles = 172;
static const uint32_t kHBlankCycles = 204;
static const uint32_t kCyclesPerLine = kSearchOAMCycles + kDataTransferCycles + kHBlankCycles; // 456
static const uint32_t kVBlankCycles = kCyclesPerLine * 10; // Lines 144 through 153
static const uint32_t kCyclesPerScreen = kCyclesPerLine * 154; // All 154 lines

Mode::Enum getCurrentMode(uint64_t cycles) {
   if (cycles < kVBlankCycles) {
      // Still in initial V-blank
      return Mode::kVBlank;
   }

   uint32_t cyclesOnScreen = (cycles - kVBlankCycles) % kCyclesPerScreen;
   uint32_t lineNumber = cyclesOnScreen / kCyclesPerLine;
   if (lineNumber >= 144) {
      return Mode::kVBlank;
   }

   uint32_t cyclesOnLine = cyclesOnScreen % kCyclesPerLine;
   if (cyclesOnLine < kSearchOAMCycles) {
      return Mode::kSearchOAM;
   } else if (cyclesOnLine < kSearchOAMCycles + kDataTransferCycles) {
      return Mode::kDataTransfer;
   } else { // if (cyclesOnLine < kSearchOAMCycles + kDataTransferCycles + kHBlankCycles)
      return Mode::kHBlank;
   }
}

uint8_t calcLY(uint64_t cycles) {
   if (cycles < kVBlankCycles) {
      // Still in initial V-blank
      cycles += kCyclesPerScreen;
   }

   uint32_t cyclesOnScreen = (cycles - kVBlankCycles) % kCyclesPerScreen;
   return cyclesOnScreen / kCyclesPerLine;
}

std::array<Pixel, 4> extractPaletteColors(uint8_t bgp) {
   // Shades of gray
   /*static const std::array<Pixel, 4> kColorValues = {
      Pixel(0x1F, 0x1F, 0x1F), // white
      Pixel(0x14, 0x14, 0x14), // light gray
      Pixel(0x0A, 0x0A, 0x0A), // dark gray
      Pixel(0x00, 0x00, 0x00)  // black
   };*/
   // Green / blue (trying to approximate original Game Boy screen colors)
   static const std::array<Pixel, 4> kColorValues = {
      Pixel(0x15, 0x19, 0x09), // white
      Pixel(0x0F, 0x15, 0x0D), // light gray
      Pixel(0x04, 0x0D, 0x0C), // dark gray - middle try E?
      Pixel(0x01, 0x05, 0x0A)  // black
   };
   static const uint8_t kMask = 0x03;

   std::array<Pixel, 4> colors;
   for (size_t i = 0; i < colors.size(); ++i) {
      size_t shift = i * 2;
      colors[i] = kColorValues[(bgp & (kMask << shift)) >> shift];
   }

   return colors;
}

} // namespace

LCDController::LCDController(Memory& memory)
   : mem(memory) {
}

void LCDController::tick(uint64_t totalCycles) {
   Mode::Enum lastMode = static_cast<Mode::Enum>(mem.stat & STAT::kModeFlag);
   Mode::Enum currentMode = getCurrentMode(totalCycles);
   // Update the mode
   mem.stat = (mem.stat & ~STAT::kModeFlag) | currentMode;

   mem.ly = calcLY(totalCycles);

   // ly compare
   bool coincident = mem.ly == mem.lyc;
   mem.stat = (mem.stat & ~STAT::kCoincidenceFlag) | (coincident ? STAT::kCoincidenceFlag : 0);
   if ((mem.stat & STAT::kLycLyCoincidence) && coincident) {
      mem.ifr |= Interrupt::kLCDState;
   }

   switch (currentMode) {
      case Mode::kHBlank:
      {
         ASSERT(lastMode == Mode::kHBlank || lastMode == Mode::kDataTransfer);

         if (lastMode != Mode::kHBlank) {
            if (mem.stat & STAT::kMode0HBlankInterrupt) {
               mem.ifr |= Interrupt::kLCDState;
            }
         }
         break;
      }
      case Mode::kVBlank:
      {
         ASSERT(lastMode == Mode::kVBlank || lastMode == Mode::kHBlank);

         if (lastMode != Mode::kVBlank) {
            mem.ifr |= Interrupt::kVBlank;

            if (mem.stat & STAT::kMode1VBlankInterrupt) {
               mem.ifr |= Interrupt::kLCDState;
            }

            framebuffers.flip();
         }
         break;
      }
      case Mode::kSearchOAM:
      {
         ASSERT(lastMode == Mode::kSearchOAM || lastMode == Mode::kHBlank || lastMode == Mode::kVBlank);

         if (lastMode != Mode::kSearchOAM) {
            if (mem.stat & STAT::kMode2OAMInterrupt) {
               mem.ifr |= Interrupt::kLCDState;
            }
         }
         break;
      }
      case Mode::kDataTransfer:
      {
         ASSERT(lastMode == Mode::kDataTransfer || lastMode == Mode::kSearchOAM);

         if (lastMode != Mode::kDataTransfer) {
            ASSERT(mem.ly < 144);
            scan(framebuffers.writeBuffer(), extractPaletteColors(mem.bgp), mem.ly);
         }
         break;
      }
      default:
         ASSERT(false);
   }
}

void LCDController::scan(Framebuffer& framebuffer, const std::array<Pixel, 4>& colors, uint8_t line) {
   if (mem.lcdc & LCDC::kDisplayEnable) {
      if (mem.lcdc & LCDC::kBGDisplay) {
         scanBackground(framebuffer, colors, line);
      }

      if (mem.lcdc & LCDC::kWindowDisplayEnable) {
         scanWindow(framebuffer, colors, line);
      }

      if (mem.lcdc & LCDC::kObjSpriteDisplayEnable) {
         scanSprites(framebuffer, colors, line);
      }
   } else {
      size_t lineOffset = line * kScreenWidth;
      for (size_t x = 0; x < kScreenWidth; ++x) {
         framebuffer[lineOffset + x] = colors[Color::kWhite];
      }
   }
}

void LCDController::scanBackground(Framebuffer& framebuffer, const std::array<Pixel, 4>& colors, uint8_t line) {
   size_t framebufferLineOffset = line * kScreenWidth;

   uint8_t row = line + mem.scy;
   for (uint8_t x = 0; x < kScreenWidth; ++x) {
      uint8_t col = x + mem.scx;

      // Fetch the tile number
      uint16_t tileMapBase = (mem.lcdc & LCDC::kBGTileMapDisplaySelect) ? 0x1C00 : 0x1800;
      uint16_t tileMapOffset = (col / 8) + 32 * (row / 8); // 32x32 tiles, 8x8 pixels each
      uint8_t tileNum = mem.vram[tileMapBase + tileMapOffset];

      // Fetch the tile data
      static const uint16_t kBytesPerTile = 16;
      uint16_t tileDataBase = (mem.lcdc & LCDC::kBGAndWindowTileDataSelect) ? 0x0000 : 0x0800;
      uint8_t tileDataLineOffset = (row % 8) * 2; // 8 lines per tile, 2 bytes per line
      uint16_t tileDataTileOffset;
      if (tileDataBase > 0) {
         // the tile number acts as a signed offset
         tileDataTileOffset = (*reinterpret_cast<int8_t*>(&tileNum) + 128) * kBytesPerTile;
      } else {
         // the tile number acts as an unsigned offset
         tileDataTileOffset = tileNum * kBytesPerTile;
      }
      uint8_t tileLineFirstByte = mem.vram[tileDataBase + tileDataTileOffset + tileDataLineOffset];
      uint8_t tileLineSecondByte = mem.vram[tileDataBase + tileDataTileOffset + tileDataLineOffset + 1];

      uint8_t bit = 7 - (col % 8); // bit 7 is the leftmost pixel, bit 0 is the rightmost pixel
      uint8_t mask = 1 << bit;

      uint8_t paletteIndex = 0;
      if (tileLineFirstByte & mask) {
         ++paletteIndex;
      }
      if (tileLineSecondByte & mask) {
         paletteIndex += 2;
      }

      framebuffer[framebufferLineOffset + x] = colors[paletteIndex];
   }
}

void LCDController::scanWindow(Framebuffer& framebuffer, const std::array<Pixel, 4>& colors, uint8_t line) {
   // TODO properly implement

   /*size_t lineOffset = line * kScreenWidth;
   for (size_t x = 0; x < kScreenWidth; ++x) {
      framebuffer[lineOffset + x] = colors[Color::kLightGray];
   }*/
}

void LCDController::scanSprites(Framebuffer& framebuffer, const std::array<Pixel, 4>& colors, uint8_t line) {
   // TODO properly implement

   /*size_t lineOffset = line * kScreenWidth;
   for (size_t x = 0; x < kScreenWidth; ++x) {
      framebuffer[lineOffset + x] = colors[Color::kDarkGray];
   }*/
}

} // namespace GBC
