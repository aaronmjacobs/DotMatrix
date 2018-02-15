#include "Log.h"

#include "GBC/CPU.h"
#include "GBC/LCDController.h"
#include "GBC/Memory.h"

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

namespace Attrib {

enum Enum : uint8_t {
   kObjToBgPriority = 1 << 7,
   kYFlip = 1 << 6,
   kXFlip = 1 << 5,
   kPaletteNumber = 1 << 4, // Non-CGB only
   kTileVRAMBank = 1 << 3, // CGB only
   kCGBPaletteNumber = (1 << 2) | (1 << 1) | (1 << 0) // CGB only
};

} // namespace Attrib

namespace ColorIndex {

enum Enum : uint8_t {
   kWhite = 0,
   kLightGray = 1,
   kDarkGray = 2,
   kBlack = 3
};

} // namespace ColorIndex

namespace Color {

// Green / blue (trying to approximate original Game Boy screen colors)
const Pixel kWhite(0xAC, 0xCD, 0x4A);
const Pixel kLightGray(0x7B, 0xAC, 0x6A);
const Pixel kDarkGray(0x20, 0x6A, 0x62);
const Pixel kBlack(0x08, 0x29, 0x52);

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

std::array<Pixel, 4> extractPaletteColors(uint8_t palette) {
   static const std::array<Pixel, 4> kColorValues = {
      Color::kWhite,
      Color::kLightGray,
      Color::kDarkGray,
      Color::kBlack,
   };
   static const uint8_t kMask = 0x03;

   std::array<Pixel, 4> colors;
   for (size_t i = 0; i < colors.size(); ++i) {
      size_t shift = i * 2;
      colors[i] = kColorValues[(palette & (kMask << shift)) >> shift];
   }

   return colors;
}

} // namespace

LCDController::LCDController(Memory& memory)
   : mem(memory), bgPaletteIndices({}) {
}

void LCDController::tick(uint64_t totalCycles, bool cpuStopped) {
   Mode::Enum lastMode = static_cast<Mode::Enum>(mem.stat & STAT::kModeFlag);
   Mode::Enum currentMode = getCurrentMode(totalCycles);
   // Update the mode
   mem.stat = (mem.stat & ~STAT::kModeFlag) | currentMode;

   uint8_t oldLY = mem.ly;
   mem.ly = calcLY(totalCycles);

   // ly compare
   bool coincident = mem.ly == mem.lyc;
   mem.stat = (mem.stat & ~STAT::kCoincidenceFlag) | (coincident ? STAT::kCoincidenceFlag : 0);
   if ((mem.stat & STAT::kLycLyCoincidence) && coincident && (mem.ly != oldLY)) {
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
            bgPaletteIndices.fill(0);
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
            scan(framebuffers.writeBuffer(), mem.ly, extractPaletteColors(mem.bgp));
         }
         break;
      }
      default:
         ASSERT(false);
   }

   // When stopped, fill the screen with white
   if (cpuStopped) {
      framebuffers.writeBuffer().fill(Color::kWhite);
      framebuffers.flip();
   }
}

void LCDController::scan(Framebuffer& framebuffer, uint8_t line, const std::array<Pixel, 4>& colors) {
   if (mem.lcdc & LCDC::kDisplayEnable) {
      if (mem.lcdc & LCDC::kBGDisplay) {
         scanBackgroundOrWindow(framebuffer, line, colors, false);
      }

      if (mem.lcdc & LCDC::kWindowDisplayEnable) {
         scanBackgroundOrWindow(framebuffer, line, colors, true);
      }

      if (mem.lcdc & LCDC::kObjSpriteDisplayEnable) {
         scanSprites(framebuffer, line);
      }
   } else {
      size_t lineOffset = line * kScreenWidth;
      for (size_t x = 0; x < kScreenWidth; ++x) {
         framebuffer[lineOffset + x] = colors[ColorIndex::kWhite];
      }
   }
}

void LCDController::scanBackgroundOrWindow(Framebuffer& framebuffer, uint8_t line, const std::array<Pixel, 4>& colors,
                                           bool isWindow) {
   // 32x32 tiles, 8x8 pixels each
   static const uint16_t kTileWidth = 8;
   static const uint16_t kTileHeight = 8;
   static const uint16_t kNumTilesPerLine = 32;
   static const uint16_t kWindowXOffset = 7;

   uint8_t y = line;
   if (isWindow && y < mem.wy) {
      // Haven't reached the window yet
      return;
   }

   uint8_t adjustedY;
   if (isWindow) {
      adjustedY = y - mem.wy;
   } else {
      adjustedY = y + mem.scy;
   }

   for (uint8_t x = 0; x < kScreenWidth; ++x) {
      if (isWindow && x < mem.wx - kWindowXOffset) {
         // Haven't reached the window yet
         continue;
      }

      uint8_t adjustedX;
      if (isWindow) {
         adjustedX = x - (mem.wx - kWindowXOffset);
      } else {
         adjustedX = x + mem.scx;
      }

      // Fetch the tile number
      uint8_t tileMapDisplaySelect = isWindow ? LCDC::kWindowTileMapDisplaySelect : LCDC::kBGTileMapDisplaySelect;
      uint16_t tileMapBase = (mem.lcdc & tileMapDisplaySelect) ? 0x1C00 : 0x1800;
      uint16_t tileMapOffset = (adjustedX / kTileWidth) + kNumTilesPerLine * (adjustedY / kTileHeight);
      uint8_t tileNum = mem.vram[tileMapBase + tileMapOffset];

      uint8_t row = adjustedY % kTileHeight;
      uint8_t col = adjustedX % kTileWidth;
      bool signedTileOffset = (mem.lcdc & LCDC::kBGAndWindowTileDataSelect) == 0x00;
      uint8_t paletteIndex = fetchPaletteIndex(tileNum, row, col, signedTileOffset, false);

      framebuffer[x + (kScreenWidth * y)] = colors[paletteIndex];
      if (!isWindow) {
         bgPaletteIndices[x + (kScreenWidth * y)] = paletteIndex;
      }
   }
}

void LCDController::scanSprites(Framebuffer& framebuffer, uint8_t line) {
   static const uint16_t kSpriteWidth = 8;
   static const uint16_t kShortSpriteHeight = 8;
   static const uint16_t kTallSpriteHeight = 16;
   static const uint16_t kNumSprites = 40;

   struct SpriteAttributes {
      uint8_t yPos;
      uint8_t xPos;
      uint8_t tileNum;
      uint8_t flags;
   };

   uint8_t y = line;
   uint8_t spriteHeight = (mem.lcdc & LCDC::kObjSpriteSize) ? kTallSpriteHeight : kShortSpriteHeight;

   for (int8_t sprite = kNumSprites - 1; sprite >= 0; --sprite) {
      SpriteAttributes attributes;
      memcpy(&attributes, &mem.sat[sprite * sizeof(SpriteAttributes)], sizeof(SpriteAttributes));

      int16_t spriteY = attributes.yPos - kTallSpriteHeight;
      if (spriteY > y || spriteY + spriteHeight <= y
         || attributes.xPos == 0 || attributes.xPos >= kScreenWidth + kSpriteWidth) {
         continue;
      }

      bool useObp1 = (attributes.flags & Attrib::kPaletteNumber) != 0x00;
      std::array<Pixel, 4> colors = extractPaletteColors(useObp1 ? mem.obp1 : mem.obp0);

      uint8_t row = y - spriteY;
      if (attributes.flags & Attrib::kYFlip) {
         row = (kTallSpriteHeight - 1) - row;
      }
      row %= spriteHeight;

      for (uint8_t col = 0; col < kSpriteWidth; ++col) {
         int16_t x = attributes.xPos - kSpriteWidth + col;
         if (x < 0 || x > kScreenWidth) {
            continue;
         }

         bool flipX = (attributes.flags & Attrib::kXFlip) != 0x00;
         uint8_t paletteIndex = fetchPaletteIndex(attributes.tileNum, row, col, false, flipX);

         // Sprite palette index 0 is transparent
         bool aboveBackground = paletteIndex != 0;

         // If the OBJ-to-BG priority bit is set, the sprite is behind background palette colors 1-3
         if (attributes.flags & Attrib::kObjToBgPriority) {
            ASSERT(bgPaletteIndices[x + (kScreenWidth * y)] <= 3);
            aboveBackground = aboveBackground && bgPaletteIndices[x + (kScreenWidth * y)] == 0;
         }

         if (aboveBackground) {
            framebuffer[x + (kScreenWidth * y)] = colors[paletteIndex];
         }
      }
   }
}

uint8_t LCDController::fetchPaletteIndex(uint8_t tileNum, uint8_t row, uint8_t col, bool signedTileOffset,
                                         bool flipX) const {
   static const uint16_t kBytesPerTile = 16;
   static const uint8_t kBytesPerLine = 2;
   static const uint16_t kSignedTileDataAddr = 0x0800;
   static const uint16_t kUnsignedTileDataAddr = 0x0000;

   uint16_t tileDataBase = signedTileOffset ? kSignedTileDataAddr : kUnsignedTileDataAddr;
   uint16_t tileDataTileOffset;
   if (signedTileOffset) {
      // treat the tile number as a signed offset
      tileDataTileOffset = (*reinterpret_cast<int8_t*>(&tileNum) + 128) * kBytesPerTile;
   } else {
      tileDataTileOffset = tileNum * kBytesPerTile;
   }
   uint8_t tileDataLineOffset = row * kBytesPerLine;
   uint8_t tileLineFirstByte = mem.vram[tileDataBase + tileDataTileOffset + tileDataLineOffset];
   uint8_t tileLineSecondByte = mem.vram[tileDataBase + tileDataTileOffset + tileDataLineOffset + 1];

   uint8_t bit = col % 8;
   if (!flipX) {
      bit = 7 - bit; // bit 7 is the leftmost pixel, bit 0 is the rightmost pixel
   }
   uint8_t mask = 1 << bit;

   uint8_t paletteIndex = 0;
   if (tileLineFirstByte & mask) {
      paletteIndex += 1;
   }
   if (tileLineSecondByte & mask) {
      paletteIndex += 2;
   }

   return paletteIndex;
}

} // namespace GBC
