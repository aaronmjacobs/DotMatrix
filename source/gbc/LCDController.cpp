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
   if (mem.stat & STAT::kLycLyCoincidence) {
      mem.stat = (mem.stat & !STAT::kCoincidenceFlag) | (mem.ly == mem.lyc ? STAT::kCoincidenceFlag : 0);

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
            scan(framebuffers.writeBuffer(), mem.ly);
         }
         break;
      }
      default:
         ASSERT(false);
   }
}

void LCDController::scan(Framebuffer& framebuffer, uint8_t line) {
   if (mem.lcdc & LCDC::kDisplayEnable) {
      if (mem.lcdc & LCDC::kBGDisplay) {
         scanBackground(framebuffer, line);
      }

      if (mem.lcdc & LCDC::kWindowDisplayEnable) {
         scanWindow(framebuffer, line);
      }

      if (mem.lcdc & LCDC::kObjSpriteDisplayEnable) {
         scanSprites(framebuffer, line);
      }
   } else {
      size_t lineOffset = line * kScreenWidth;
      for (size_t x = 0; x < kScreenWidth; ++x) {
         framebuffer[lineOffset + x] = {};
      }
   }
}

void LCDController::scanBackground(Framebuffer& framebuffer, uint8_t line) {
   size_t framebufferLineOffset = line * kScreenWidth;

   uint8_t row = line - mem.scy;

   for (uint8_t x = 0; x < kScreenWidth; ++x) {
      uint8_t col = x - mem.scx;

      uint16_t tileMapIndex = (col / 32) + 8 * (row / 32);
      uint16_t tileMapOffset = 0x1800 + tileMapIndex;
      uint8_t tileNum = mem.vram[tileMapOffset];
      uint16_t tileOffset = tileNum * 16; // 16 bytes per tile

      uint8_t lineOffset = (row % 8) * 2; // 8 lines per tile, 2 bytes per line

      uint8_t tileLineFirstHalf = mem.vram[tileOffset + lineOffset];
      uint8_t tileLineSecondHalf = mem.vram[tileOffset + lineOffset + 1];

      uint8_t bit = col % 8;
      uint8_t mask = 1 << bit;

      uint8_t pixelVal = 0;
      if (tileLineFirstHalf & mask) {
         ++pixelVal;
      }
      if (tileLineSecondHalf & mask) {
         ++pixelVal;
      }

      uint8_t outColor = 0x00;
      switch (pixelVal) {
         case 0:
            outColor = 0x1F;
            break;
         case 1:
            outColor = 0x14;
            break;
         case 2:
            outColor = 0x0A;
            break;
         case 3:
            outColor = 0x00;
            break;
      }
      framebuffer[framebufferLineOffset + x].r = outColor;
      framebuffer[framebufferLineOffset + x].g = outColor;
      framebuffer[framebufferLineOffset + x].b = outColor;
   }
}

void LCDController::scanWindow(Framebuffer& framebuffer, uint8_t line) {
   // TODO properly implement

   size_t lineOffset = line * kScreenWidth;
   for (size_t x = 0; x < kScreenWidth; ++x) {
      framebuffer[lineOffset + x].g = 0x14;
   }
}

void LCDController::scanSprites(Framebuffer& framebuffer, uint8_t line) {
   // TODO properly implement
   
   size_t lineOffset = line * kScreenWidth;
   for (size_t x = 0; x < kScreenWidth; ++x) {
      framebuffer[lineOffset + x].b = 0x0A;
   }
}

} // namespace GBC
