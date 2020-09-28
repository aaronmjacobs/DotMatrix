#include "Core/Log.h"
#include "Core/Math.h"

#include "GameBoy/CPU.h"
#include "GameBoy/LCDController.h"
#include "GameBoy/GameBoy.h"

#include <array>

namespace DotMatrix
{

namespace
{
   namespace LCDC
   {
      enum Enum : uint8_t
      {
         DisplayEnable = 1 << 7,
         WindowTileMapDisplaySelect = 1 << 6,
         WindowDisplayEnable = 1 << 5,
         BGAndWindowTileDataSelect = 1 << 4,
         BGTileMapDisplaySelect = 1 << 3,
         ObjSpriteSize = 1 << 2,
         ObjSpriteDisplayEnable = 1 << 1,
         BGDisplay = 1 << 0
      };
   }

   namespace STAT
   {
      enum Enum : uint8_t
      {
         LycLyCoincidence = 1 << 6,
         Mode2OAMInterrupt = 1 << 5,
         Mode1VBlankInterrupt = 1 << 4,
         Mode0HBlankInterrupt = 1 << 3,
         CoincidenceFlag = 1 << 2,
         ModeFlag = (1 << 1) | (1 << 0)
      };
   }

   namespace Attrib
   {
      enum Enum : uint8_t
      {
         ObjToBgPriority = 1 << 7,
         YFlip = 1 << 6,
         XFlip = 1 << 5,
         PaletteNumber = 1 << 4, // Non-CGB only
         TileVRAMBank = 1 << 3, // CGB only
         CGBPaletteNumber = (1 << 2) | (1 << 1) | (1 << 0) // CGB only
      };
   }

   const uint32_t kSearchOAMCycles = 80;
   const uint32_t kDataTransferCycles = 172;
   const uint32_t kHBlankCycles = 204;
   const uint32_t kCyclesPerLine = kSearchOAMCycles + kDataTransferCycles + kHBlankCycles; // 456
}

uint8_t LCDController::ControlRegister::read() const
{
   return lcdDisplayEnabled * LCDC::DisplayEnable
      | windowUseUpperTileMap * LCDC::WindowTileMapDisplaySelect
      | windowDisplayEnabled * LCDC::WindowDisplayEnable
      | bgAndWindowUseUnsignedTileData * LCDC::BGAndWindowTileDataSelect
      | bgUseUpperTileMap * LCDC::BGTileMapDisplaySelect
      | useLargeSpriteSize * LCDC::ObjSpriteSize
      | spriteDisplayEnabled * LCDC::ObjSpriteDisplayEnable
      | bgWindowDisplayEnabled * LCDC::BGDisplay;
}

void LCDController::ControlRegister::write(uint8_t value)
{
   lcdDisplayEnabled = value & LCDC::DisplayEnable;
   windowUseUpperTileMap = value & LCDC::WindowTileMapDisplaySelect;
   windowDisplayEnabled = value & LCDC::WindowDisplayEnable;
   bgAndWindowUseUnsignedTileData = value & LCDC::BGAndWindowTileDataSelect;
   bgUseUpperTileMap = value & LCDC::BGTileMapDisplaySelect;
   useLargeSpriteSize = value & LCDC::ObjSpriteSize;
   spriteDisplayEnabled = value & LCDC::ObjSpriteDisplayEnable;
   bgWindowDisplayEnabled = value & LCDC::BGDisplay;
}

uint8_t LCDController::StatusRegister::read() const
{
   return 0x80
      | coincidenceInterrupt * STAT::LycLyCoincidence
      | oamInterrupt * STAT::Mode2OAMInterrupt
      | vBlankInterrupt * STAT::Mode1VBlankInterrupt
      | hBlankInterrupt * STAT::Mode0HBlankInterrupt
      | coincidenceFlag * STAT::CoincidenceFlag
      | Enum::cast(mode);
}

void LCDController::StatusRegister::write(uint8_t value)
{
   coincidenceInterrupt = value & STAT::LycLyCoincidence;
   oamInterrupt = value & STAT::Mode2OAMInterrupt;
   vBlankInterrupt = value & STAT::Mode1VBlankInterrupt;
   hBlankInterrupt = value & STAT::Mode0HBlankInterrupt;
   coincidenceFlag = value & STAT::CoincidenceFlag;
   // Mode flag should be unaffected by memory writes
}

LCDController::LCDController(GameBoy& gb)
   : gameBoy(gb)
   , modeCyclesRemaining(kCyclesPerLine)
{
}

void LCDController::onCPUStopped()
{
   // When stopped, fill the screen with white
   framebuffers.writeBuffer().fill(0x00);
}

uint8_t LCDController::read(uint16_t address) const
{
   uint8_t value = GameBoy::kInvalidAddressByte;

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
         value = controlRegister.read();
         break;
      case 0xFF41: // LCD status
         value = statusRegister.read();
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
         DM_ASSERT(false);
         break;
      }
   }
   else
   {
      DM_ASSERT(false);
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
         controlRegister.write(value);
         break;
      case 0xFF41: // LCD status
         statusRegister.write(value);
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
         DM_ASSERT(false);
         break;
      }
   }
   else
   {
      DM_ASSERT(false);
   }
}

std::array<uint8_t, 4> LCDController::extractPaletteColors(uint8_t palette) const
{
   static const uint8_t kMask = 0x03;

   std::array<uint8_t, 4> colors;
   for (std::size_t i = 0; i < colors.size(); ++i)
   {
      std::size_t shift = i * 2;
      colors[i] = (palette & (kMask << shift)) >> shift;
   }

   return colors;
}

void LCDController::updateDMA()
{
   if (dmaPending)
   {
      dmaPending = false;

      dmaInProgress = true;
      dmaIndex = 0x00;

      DM_ASSERT(dma <= 0xF1);
      dmaSource = dma << 8;
   }

   if (dmaInProgress)
   {
      if (dmaIndex <= 0x9F)
      {
         oam[dmaIndex] = gameBoy.readDirect(dmaSource + dmaIndex);
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
      dmaPending = true;
   }
}

void LCDController::updateMode()
{
   DM_STATIC_ASSERT(kHBlankCycles % CPU::kClockCyclesPerMachineCycle == 0
      && kCyclesPerLine % CPU::kClockCyclesPerMachineCycle == 0
      && kSearchOAMCycles % CPU::kClockCyclesPerMachineCycle == 0
      && kDataTransferCycles % CPU::kClockCyclesPerMachineCycle == 0
      , "Mode cycles not evenly divisible by CPU::kClockCyclesPerMachineCycle");

   static const std::array<uint32_t, 4> kModeCycles = { kHBlankCycles, kCyclesPerLine, kSearchOAMCycles, kDataTransferCycles };

   modeCyclesRemaining -= CPU::kClockCyclesPerMachineCycle;
   if (modeCyclesRemaining == 0)
   {
      Mode lastMode = statusRegister.mode;
      Mode currentMode = lastMode;

      switch (lastMode)
      {
      case Mode::HBlank:
         ++ly;

         if (ly < 144)
         {
            currentMode = Mode::SearchOAM;
         }
         else
         {
            currentMode = Mode::VBlank;
         }

         updateLYC();
         break;
      case Mode::VBlank:
         ++ly;

         if (ly < 154)
         {
            currentMode = Mode::VBlank;
         }
         else
         {
            ly = 0;
            currentMode = Mode::SearchOAM;
         }

         updateLYC();
         break;
      case Mode::SearchOAM:
         currentMode = Mode::DataTransfer;
         break;
      case Mode::DataTransfer:
         currentMode = Mode::HBlank;
         break;
      default:
         DM_ASSERT(false);
         break;
      }

      if (lastMode != currentMode)
      {
         setMode(currentMode);
      }

      modeCyclesRemaining = kModeCycles[Enum::cast(currentMode)];
   }
}

void LCDController::updateLYC()
{
   statusRegister.coincidenceFlag = ly == lyc;

   if (statusRegister.coincidenceFlag && statusRegister.coincidenceInterrupt)
   {
      gameBoy.requestInterrupt(Interrupt::LCDState);
   }
}

void LCDController::setMode(Mode newMode)
{
   DM_ASSERT(modeCyclesRemaining == 0);
   statusRegister.mode = newMode;

   switch (newMode)
   {
   case Mode::HBlank:
      if (statusRegister.hBlankInterrupt)
      {
         gameBoy.requestInterrupt(Interrupt::LCDState);
      }
      break;
   case Mode::VBlank:
      gameBoy.requestInterrupt(Interrupt::VBlank);

      if (statusRegister.vBlankInterrupt)
      {
         gameBoy.requestInterrupt(Interrupt::LCDState);
      }

      // If bit 5 (mode 2 OAM interrupt) is set, an interrupt is also triggered at line 144 when vblank starts
      if (statusRegister.oamInterrupt)
      {
         gameBoy.requestInterrupt(Interrupt::LCDState);
      }

      framebuffers.flip();
      bgPaletteIndices.fill(0);
      break;
   case Mode::SearchOAM:
      if (statusRegister.oamInterrupt)
      {
         gameBoy.requestInterrupt(Interrupt::LCDState);
      }
      break;
   case Mode::DataTransfer:
      DM_ASSERT(ly < 144);

      scan(framebuffers.writeBuffer(), ly, extractPaletteColors(bgp));
      break;
   default:
      DM_ASSERT(false);
      break;
   }
}

void LCDController::scan(Framebuffer& framebuffer, uint8_t line, const std::array<uint8_t, 4>& paletteColors)
{
   if (controlRegister.lcdDisplayEnabled)
   {
      if (controlRegister.bgWindowDisplayEnabled)
      {
         scanBackgroundOrWindow<false>(framebuffer, line, paletteColors);
      }

      if (controlRegister.bgWindowDisplayEnabled && controlRegister.windowDisplayEnabled)
      {
         scanBackgroundOrWindow<true>(framebuffer, line, paletteColors);
      }

      if (controlRegister.spriteDisplayEnabled)
      {
         scanSprites(framebuffer, line);
      }
   }
   else
   {
      size_t lineOffset = line * kScreenWidth;
      for (size_t x = 0; x < kScreenWidth; ++x)
      {
         framebuffer[lineOffset + x] = 0x00;
      }
   }
}

template<bool isWindow>
void LCDController::scanBackgroundOrWindow(Framebuffer& framebuffer, uint8_t line, const std::array<uint8_t, 4>& paletteColors)
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

   int16_t yOffset = isWindow ? -wy : scy;
   int16_t xOffset = isWindow ? (kWindowXOffset - wx) : scx;

   uint8_t adjustedY = y + yOffset;
   uint8_t row = adjustedY % kTileHeight;
   uint16_t tileMapYOffset = (adjustedY / kTileHeight) * kNumTilesPerLine;

   bool tileMapDisplaySelect = isWindow ? controlRegister.windowUseUpperTileMap : controlRegister.bgUseUpperTileMap;
   uint16_t tileMapBase = tileMapDisplaySelect ? 0x1C00 : 0x1800;
   bool signedTileOffset = !controlRegister.bgAndWindowUseUnsignedTileData;

   uint16_t pixelYOffset = kScreenWidth * y;

   uint8_t x = 0;
   if (isWindow && xOffset < 0)
   {
      x -= xOffset;
   }

   while (x < kScreenWidth)
   {
      uint8_t adjustedX = x + xOffset;
      uint8_t tileMapXOffset = adjustedX / kTileWidth;
      uint8_t col = adjustedX % kTileWidth;

      uint16_t tileMapOffset = tileMapXOffset + tileMapYOffset;
      uint8_t tileNum = vram[tileMapBase + tileMapOffset];
      TileLine tileLine = fetchTileLine(tileNum, row, signedTileOffset);

      for (; col < kTileWidth && x < kScreenWidth; ++col, ++x)
      {
         uint8_t mask = (0b10000000 >> col); // bit 7 is the leftmost pixel, bit 0 is the rightmost pixel
         uint8_t paletteIndex = static_cast<bool>(tileLine.firstByte & mask) + 2 * static_cast<bool>(tileLine.secondByte & mask);

         uint16_t pixel = x + pixelYOffset;
         framebuffer[pixel] = paletteColors[paletteIndex];
         if (!isWindow)
         {
            bgPaletteIndices[pixel] = paletteColors[paletteIndex];
         }
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
   uint8_t spriteHeight = controlRegister.useLargeSpriteSize ? kTallSpriteHeight : kShortSpriteHeight;
   uint16_t pixelYOffset = kScreenWidth * y;

   for (int8_t sprite = kNumSprites - 1; sprite >= 0; --sprite)
   {
      SpriteAttributes attributes = spriteAttributes[sprite];

      int16_t spriteY = attributes.yPos - kTallSpriteHeight;
      if (spriteY > y || spriteY + spriteHeight <= y
         || attributes.xPos == 0 || attributes.xPos >= kScreenWidth + kSpriteWidth)
      {
         continue;
      }

      bool useObp1 = (attributes.flags & Attrib::PaletteNumber) != 0x00;
      std::array<uint8_t, 4> paletteColors = extractPaletteColors(useObp1 ? obp1 : obp0);

      uint8_t row = y - spriteY;
      if (attributes.flags & Attrib::YFlip)
      {
         row = (kTallSpriteHeight - 1) - row;
      }
      row %= spriteHeight;

      bool flipX = (attributes.flags & Attrib::XFlip) != 0x00;
      TileLine tileLine = fetchTileLine(attributes.tileNum, row, false);

      for (uint8_t col = 0; col < kSpriteWidth; ++col)
      {
         int16_t x = attributes.xPos - kSpriteWidth + col;
         if (x < 0 || x > kScreenWidth)
         {
            continue;
         }

         uint16_t pixel = x + pixelYOffset;

         uint8_t mask = flipX ? (0b00000001 << col) : (0b10000000 >> col); // bit 7 is the leftmost pixel, bit 0 is the rightmost pixel
         uint8_t paletteIndex = static_cast<bool>(tileLine.firstByte & mask) + 2 * static_cast<bool>(tileLine.secondByte & mask);

         // Sprite palette index 0 is transparent
         bool aboveBackground = paletteIndex != 0;

         // If the OBJ-to-BG priority bit is set, the sprite is behind background palette colors 1-3
         if (attributes.flags & Attrib::ObjToBgPriority)
         {
            DM_ASSERT(bgPaletteIndices[pixel] <= 3);
            aboveBackground = aboveBackground && bgPaletteIndices[pixel] == 0;
         }

         if (aboveBackground)
         {
            framebuffer[pixel] = paletteColors[paletteIndex];
         }
      }
   }
}

LCDController::TileLine LCDController::fetchTileLine(uint8_t tileNum, uint8_t line, bool signedTileOffset) const
{
   static const uint8_t kBytesPerTile = 16;
   static const uint8_t kBytesPerLine = 2;
   static const uint16_t kSignedTileDataAddr = 0x0800;
   static const uint16_t kUnsignedTileDataAddr = 0x0000;

   uint16_t tileDataBase;
   uint16_t tileDataTileOffset;
   if (signedTileOffset)
   {
      tileDataBase = kSignedTileDataAddr;
      tileDataTileOffset = (Math::reinterpretAsSigned(tileNum) + 128) * kBytesPerTile;
   }
   else
   {
      tileDataBase = kUnsignedTileDataAddr;
      tileDataTileOffset = tileNum * kBytesPerTile;
   }
   uint8_t tileDataLineOffset = line * kBytesPerLine;
   uint16_t totalOffset = tileDataBase + tileDataTileOffset + tileDataLineOffset;

   TileLine tileLine;
   tileLine.firstByte = vram[totalOffset];
   tileLine.secondByte = vram[totalOffset + 1];

   return tileLine;
}

} // namespace DotMatrix
