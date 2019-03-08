#include "Core/Log.h"

#include "GBC/CPU.h"
#include "GBC/LCDController.h"
#include "GBC/GameBoy.h"
#include "GBC/Memory.h"

#include <array>

namespace GBC
{

namespace
{

namespace LCDC
{

enum Enum : uint8_t
{
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

namespace STAT
{

enum Enum : uint8_t
{
   kLycLyCoincidence = 1 << 6,
   kMode2OAMInterrupt = 1 << 5,
   kMode1VBlankInterrupt = 1 << 4,
   kMode0HBlankInterrupt = 1 << 3,
   kCoincidenceFlag = 1 << 2,
   kModeFlag = (1 << 1) | (1 << 0)
};

} // namespace STAT

namespace Attrib
{

enum Enum : uint8_t
{
   kObjToBgPriority = 1 << 7,
   kYFlip = 1 << 6,
   kXFlip = 1 << 5,
   kPaletteNumber = 1 << 4, // Non-CGB only
   kTileVRAMBank = 1 << 3, // CGB only
   kCGBPaletteNumber = (1 << 2) | (1 << 1) | (1 << 0) // CGB only
};

} // namespace Attrib

namespace ColorIndex
{

enum Enum : uint8_t
{
   kWhite = 0,
   kLightGray = 1,
   kDarkGray = 2,
   kBlack = 3
};

} // namespace ColorIndex

namespace Color
{

// Green / blue (trying to approximate original Game Boy screen colors)
const Pixel kWhite(0xAC, 0xCD, 0x4A);
const Pixel kLightGray(0x7B, 0xAC, 0x6A);
const Pixel kDarkGray(0x20, 0x6A, 0x62);
const Pixel kBlack(0x08, 0x29, 0x52);

} // namespace Color

std::array<Pixel, 4> extractPaletteColors(uint8_t palette)
{
   static const std::array<Pixel, 4> kColorValues =
   {
      Color::kWhite,
      Color::kLightGray,
      Color::kDarkGray,
      Color::kBlack,
   };
   static const uint8_t kMask = 0x03;

   std::array<Pixel, 4> colors;
   for (size_t i = 0; i < colors.size(); ++i)
   {
      size_t shift = i * 2;
      colors[i] = kColorValues[(palette & (kMask << shift)) >> shift];
   }

   return colors;
}

const uint32_t kSearchOAMCycles = 80;
const uint32_t kDataTransferCycles = 172;
const uint32_t kHBlankCycles = 204;
const uint32_t kCyclesPerLine = kSearchOAMCycles + kDataTransferCycles + kHBlankCycles; // 456

} // namespace

LCDController::LCDController(GameBoy& gb)
   : gameBoy(gb)
   , modeCyclesRemaining(kCyclesPerLine)
   , dmaRequested(false)
   , dmaInProgress(false)
   , dmaIndex(0)
   , dmaSource(0)
   , lcdc(0)
   , stat(Mode::kVBlank)
   , scy(0)
   , scx(0)
   , ly(144)
   , lyc(0)
   , dma(0)
   , bgp(0)
   , obp0(0)
   , obp1(0)
   , wy(0)
   , wx(0)
   , vram{}
   , oam{}
   , bgPaletteIndices{}
{
}

void LCDController::machineCycle()
{
   updateDMA();
   updateMode();
}

uint8_t LCDController::read(uint16_t address) const
{
   uint8_t value = Memory::kInvalidAddressByte;

   if (address >= 0x8000 && address <= 0x9FFF)
   {
      value = vram[address - 0x8000];
   }
   else if (address >= 0xFE00 && address <= 0xFEFF)
   {
      // Can't read during OAM DMA
      if (isSpriteAttributeTableAccessible())
      {
         value = oam[address - 0xFE00];
      }
   }
   else if (address >= 0xFF40 && address <= 0xFF4F)
   {
      switch (address)
      {
      case 0xFF40: // LCD control
         value = lcdc;
         break;
      case 0xFF41: // LCD status
         value = stat;
         break;
      case 0xFF42: // Scroll y
         value = scy;
         break;
      case 0xFF43: // Scroll x
         value = scx;
         break;
      case 0xFF44: // LCDC y-coordinate
         value = ly;
         break;
      case 0xFF45: // LY compare
         value = lyc;
         break;
      case 0xFF46: // DMA transfer & start address
         value = dma;
         break;
      case 0xFF47: // BG & window palette data
         value = bgp;
         break;
      case 0xFF48: // Object palette 0 data
         value = obp0;
         break;
      case 0xFF49: // Object palette 1 data
         value = obp1;
         break;
      case 0xFF4A: // Window y position
         value = wy;
         break;
      case 0xFF4B: // Window x position
         value = wx;
         break;
      case 0xFF4C: // Padding
      case 0xFF4D: // Padding
      case 0xFF4E: // Padding
      case 0xFF4F: // Padding
         break;
      default:
         ASSERT(false);
         break;
      }
   }
   else
   {
      ASSERT(false);
   }

   return value;
}

void LCDController::write(uint16_t address, uint8_t value)
{
   if (address >= 0x8000 && address <= 0x9FFF)
   {
      vram[address - 0x8000] = value;
   }
   else if (address >= 0xFE00 && address <= 0xFEFF)
   {
      // Can't write during OAM DMA
      if (isSpriteAttributeTableAccessible())
      {
         oam[address - 0xFE00] = value;
      }
   }
   else if (address >= 0xFF40 && address <= 0xFF4F)
   {
      switch (address)
      {
      case 0xFF40: // LCD control
         lcdc = value;
         break;
      case 0xFF41: // LCD status
         stat = value;
         break;
      case 0xFF42: // Scroll y
         scy = value;
         break;
      case 0xFF43: // Scroll x
         scx = value;
         break;
      case 0xFF44: // LCDC y-coordinate
         ly = value;
         updateLYC();
         break;
      case 0xFF45: // LY compare
         lyc = value;
         updateLYC();
         break;
      case 0xFF46: // DMA transfer & start address
         dma = value;
         dmaRequested = true;
         break;
      case 0xFF47: // BG & window palette data
         bgp = value;
         break;
      case 0xFF48: // Object palette 0 data
         obp0 = value;
         break;
      case 0xFF49: // Object palette 1 data
         obp1 = value;
         break;
      case 0xFF4A: // Window y position
         wy = value;
         break;
      case 0xFF4B: // Window x position
         wx = value;
         break;
      case 0xFF4C: // Padding
      case 0xFF4D: // Padding
      case 0xFF4E: // Padding
      case 0xFF4F: // Padding
         break;
      default:
         ASSERT(false);
         break;
      }
   }
   else
   {
      ASSERT(false);
   }
}

void LCDController::updateDMA()
{
   if (dmaInProgress)
   {
      if (dmaIndex <= 0x9F)
      {
         oam[dmaIndex] = gameBoy.getMemory().readDirect(dmaSource + dmaIndex);
         ++dmaIndex;
      }
      else
      {
         dmaInProgress = false;
         dmaIndex = 0x00;
      }
   }

   if (dmaRequested)
   {
      dmaRequested = false;
      dmaInProgress = true;
      dmaIndex = 0x00;

      ASSERT(dma <= 0xF1);
      dmaSource = dma << 8;
   }
}

void LCDController::updateMode()
{
   static_assert(kHBlankCycles % CPU::kClockCyclesPerMachineCycle == 0
      && kCyclesPerLine % CPU::kClockCyclesPerMachineCycle == 0
      && kSearchOAMCycles % CPU::kClockCyclesPerMachineCycle == 0
      && kDataTransferCycles % CPU::kClockCyclesPerMachineCycle == 0
      , "Mode cycles not evenly divisible by CPU::kClockCyclesPerMachineCycle");

   static const std::array<uint32_t, 4> kModeCycles = { kHBlankCycles, kCyclesPerLine, kSearchOAMCycles, kDataTransferCycles };

   modeCyclesRemaining -= CPU::kClockCyclesPerMachineCycle;
   if (modeCyclesRemaining == 0)
   {
      Mode::Enum lastMode = static_cast<Mode::Enum>(stat & STAT::kModeFlag);
      Mode::Enum currentMode = lastMode;

      switch (lastMode)
      {
      case Mode::kHBlank:
         ++ly;

         if (ly < 144)
         {
            currentMode = Mode::kSearchOAM;
         }
         else
         {
            currentMode = Mode::kVBlank;
         }

         updateLYC();
         break;
      case Mode::kVBlank:
         ++ly;

         if (ly < 154)
         {
            currentMode = Mode::kVBlank;
         }
         else
         {
            ly = 0;
            currentMode = Mode::kSearchOAM;
         }

         updateLYC();
         break;
      case Mode::kSearchOAM:
         currentMode = Mode::kDataTransfer;
         break;
      case Mode::kDataTransfer:
         currentMode = Mode::kHBlank;
         break;
      default:
         ASSERT(false);
         break;
      }

      if (lastMode != currentMode)
      {
         setMode(currentMode);
      }

      modeCyclesRemaining = kModeCycles[currentMode];
   }
}

void LCDController::updateLYC()
{
   bool coincident = ly == lyc;
   stat = (stat & ~STAT::kCoincidenceFlag) | (coincident ? STAT::kCoincidenceFlag : 0);

   if (coincident && (stat & STAT::kLycLyCoincidence))
   {
      Memory& mem = gameBoy.getMemory();
      mem.ifr |= Interrupt::kLCDState;
   }
}

void LCDController::setMode(Mode::Enum newMode)
{
   ASSERT(modeCyclesRemaining == 0);
   stat = (stat & ~STAT::kModeFlag) | newMode;

   Memory& mem = gameBoy.getMemory();
   switch (newMode)
   {
   case Mode::kHBlank:
      if (stat & STAT::kMode0HBlankInterrupt)
      {
         mem.ifr |= Interrupt::kLCDState;
      }
      break;
   case Mode::kVBlank:
      mem.ifr |= Interrupt::kVBlank;

      if (stat & STAT::kMode1VBlankInterrupt)
      {
         mem.ifr |= Interrupt::kLCDState;
      }

      // When stopped, fill the screen with white
      if (gameBoy.getCPU().isStopped())
      {
         framebuffers.writeBuffer().fill(Color::kWhite);
      }

      framebuffers.flip();
      bgPaletteIndices.fill(0);
      break;
   case Mode::kSearchOAM:
      if (stat & STAT::kMode2OAMInterrupt)
      {
         mem.ifr |= Interrupt::kLCDState;
      }
      break;
   case Mode::kDataTransfer:
      ASSERT(ly < 144);

      scan(framebuffers.writeBuffer(), ly, extractPaletteColors(bgp));
      break;
   default:
      ASSERT(false);
      break;
   }
}

void LCDController::scan(Framebuffer& framebuffer, uint8_t line, const std::array<Pixel, 4>& colors)
{
   if (lcdc & LCDC::kDisplayEnable)
   {
      if (lcdc & LCDC::kBGDisplay)
      {
         scanBackgroundOrWindow<false>(framebuffer, line, colors);
      }

      if (lcdc & LCDC::kWindowDisplayEnable)
      {
         scanBackgroundOrWindow<true>(framebuffer, line, colors);
      }

      if (lcdc & LCDC::kObjSpriteDisplayEnable)
      {
         scanSprites(framebuffer, line);
      }
   }
   else
   {
      size_t lineOffset = line * kScreenWidth;
      for (size_t x = 0; x < kScreenWidth; ++x)
      {
         framebuffer[lineOffset + x] = colors[ColorIndex::kWhite];
      }
   }
}

template<bool isWindow>
void LCDController::scanBackgroundOrWindow(Framebuffer& framebuffer, uint8_t line, const std::array<Pixel, 4>& colors)
{
   // 32x32 tiles, 8x8 pixels each
   static const uint16_t kTileWidth = 8;
   static const uint16_t kTileHeight = 8;
   static const uint16_t kNumTilesPerLine = 32;
   static const uint16_t kWindowXOffset = 7;

   uint8_t y = line;
   if (isWindow && y < wy)
   {
      // Haven't reached the window yet
      return;
   }

   uint8_t adjustedY;
   if (isWindow)
   {
      adjustedY = y - wy;
   }
   else
   {
      adjustedY = y + scy;
   }

   for (uint8_t x = 0; x < kScreenWidth; ++x)
   {
      if (isWindow && x < wx - kWindowXOffset)
      {
         // Haven't reached the window yet
         continue;
      }

      uint8_t adjustedX;
      if (isWindow)
      {
         adjustedX = x - (wx - kWindowXOffset);
      }
      else
      {
         adjustedX = x + scx;
      }

      // Fetch the tile number
      uint8_t tileMapDisplaySelect = isWindow ? LCDC::kWindowTileMapDisplaySelect : LCDC::kBGTileMapDisplaySelect;
      uint16_t tileMapBase = (lcdc & tileMapDisplaySelect) ? 0x1C00 : 0x1800;
      uint16_t tileMapOffset = (adjustedX / kTileWidth) + kNumTilesPerLine * (adjustedY / kTileHeight);
      uint8_t tileNum = vram[tileMapBase + tileMapOffset];

      uint8_t row = adjustedY % kTileHeight;
      uint8_t col = adjustedX % kTileWidth;
      bool signedTileOffset = (lcdc & LCDC::kBGAndWindowTileDataSelect) == 0x00;
      uint8_t paletteIndex = fetchPaletteIndex(tileNum, row, col, signedTileOffset, false);

      framebuffer[x + (kScreenWidth * y)] = colors[paletteIndex];
      if (!isWindow)
      {
         bgPaletteIndices[x + (kScreenWidth * y)] = paletteIndex;
      }
   }
}

void LCDController::scanSprites(Framebuffer& framebuffer, uint8_t line)
{
   static const uint16_t kSpriteWidth = 8;
   static const uint16_t kShortSpriteHeight = 8;
   static const uint16_t kTallSpriteHeight = 16;
   static const uint16_t kNumSprites = 40;

   uint8_t y = line;
   uint8_t spriteHeight = (lcdc & LCDC::kObjSpriteSize) ? kTallSpriteHeight : kShortSpriteHeight;

   for (int8_t sprite = kNumSprites - 1; sprite >= 0; --sprite)
   {
      SpriteAttributes attributes = spriteAttributes[sprite];

      int16_t spriteY = attributes.yPos - kTallSpriteHeight;
      if (spriteY > y || spriteY + spriteHeight <= y
         || attributes.xPos == 0 || attributes.xPos >= kScreenWidth + kSpriteWidth)
      {
         continue;
      }

      bool useObp1 = (attributes.flags & Attrib::kPaletteNumber) != 0x00;
      std::array<Pixel, 4> colors = extractPaletteColors(useObp1 ? obp1 : obp0);

      uint8_t row = y - spriteY;
      if (attributes.flags & Attrib::kYFlip)
      {
         row = (kTallSpriteHeight - 1) - row;
      }
      row %= spriteHeight;

      for (uint8_t col = 0; col < kSpriteWidth; ++col)
      {
         int16_t x = attributes.xPos - kSpriteWidth + col;
         if (x < 0 || x > kScreenWidth)
         {
            continue;
         }

         bool flipX = (attributes.flags & Attrib::kXFlip) != 0x00;
         uint8_t paletteIndex = fetchPaletteIndex(attributes.tileNum, row, col, false, flipX);

         // Sprite palette index 0 is transparent
         bool aboveBackground = paletteIndex != 0;

         // If the OBJ-to-BG priority bit is set, the sprite is behind background palette colors 1-3
         if (attributes.flags & Attrib::kObjToBgPriority)
         {
            ASSERT(bgPaletteIndices[x + (kScreenWidth * y)] <= 3);
            aboveBackground = aboveBackground && bgPaletteIndices[x + (kScreenWidth * y)] == 0;
         }

         if (aboveBackground)
         {
            framebuffer[x + (kScreenWidth * y)] = colors[paletteIndex];
         }
      }
   }
}

uint8_t LCDController::fetchPaletteIndex(uint8_t tileNum, uint8_t row, uint8_t col, bool signedTileOffset,
                                         bool flipX) const
{
   static const uint16_t kBytesPerTile = 16;
   static const uint8_t kBytesPerLine = 2;
   static const uint16_t kSignedTileDataAddr = 0x0800;
   static const uint16_t kUnsignedTileDataAddr = 0x0000;

   uint16_t tileDataBase = signedTileOffset ? kSignedTileDataAddr : kUnsignedTileDataAddr;
   uint16_t tileDataTileOffset;
   if (signedTileOffset)
   {
      // treat the tile number as a signed offset
      tileDataTileOffset = (*reinterpret_cast<int8_t*>(&tileNum) + 128) * kBytesPerTile;
   }
   else
   {
      tileDataTileOffset = tileNum * kBytesPerTile;
   }
   uint8_t tileDataLineOffset = row * kBytesPerLine;
   uint8_t tileLineFirstByte = vram[tileDataBase + tileDataTileOffset + tileDataLineOffset];
   uint8_t tileLineSecondByte = vram[tileDataBase + tileDataTileOffset + tileDataLineOffset + 1];

   uint8_t bit = col % 8;
   if (!flipX)
   {
      bit = 7 - bit; // bit 7 is the leftmost pixel, bit 0 is the rightmost pixel
   }
   uint8_t mask = 1 << bit;

   uint8_t paletteIndex = 0;
   if (tileLineFirstByte & mask)
   {
      paletteIndex += 1;
   }
   if (tileLineSecondByte & mask)
   {
      paletteIndex += 2;
   }

   return paletteIndex;
}

} // namespace GBC
