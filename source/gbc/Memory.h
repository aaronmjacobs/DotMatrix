#ifndef GBC_MEMORY_H
#define GBC_MEMORY_H

#include "FancyAssert.h"

#include <cstdint>

namespace GBC {

namespace Interrupt {

enum Enum : uint8_t {
   kVBlank = 1 << 0,
   kLCDState = 1 << 1,
   kTimer = 1 << 2,
   kSerial = 1 << 3,
   kJoypad = 1 << 4,
};

} // namespace Interrupt

class Memory {
public:
   Memory();

   union {
      uint8_t raw[0x10000];      // Direct access                (0x0000-0xFFFF, 64k)

      struct {
         uint8_t romb[0x4000];   // Permanently-mapped ROM bank  (0x0000-0x3FFF, 16k)
         uint8_t roms[0x4000];   // Switchable ROM bank          (0x4000-0x7FFF, 16k)
         uint8_t vram[0x2000];   // Video RAM                    (0x8000-0x9FFF, 8k)
         uint8_t eram[0x2000];   // Switchable external RAM bank (0xA000-0xBFFF, 8k)
         uint8_t ram0[0x1000];   // Working RAM bank 0           (0xC000-0xCFFF, 4k)
         uint8_t ram1[0x1000];   // Working RAM bank 1           (0xD000-0xDFFF, 4k)
         uint8_t ramm[0x1E00];   // Mirror of working ram        (0xE000-0xFDFF, 7680)
         uint8_t sat[0x100];     // Sprite attribute table       (0xFE00-0xFEFF, 256)

         union {                 // I/O device mappings          (0xFF00-0xFF7F, 128)
            uint8_t io[0x80];    // Direct access                (0xFF00-0xFF7F, 128)

            struct {
               // General
               uint8_t p1;       // Joy pad / system info        (0xFF00, 1)
               uint8_t sb;       // Serial transfer data         (0xFF01, 1)
               uint8_t sc;       // IO control                   (0xFF02, 1)
               uint8_t pad0[1];  // Padding                      (0xFF03, 1)
               uint8_t div;      // Divider register             (0xFF04, 1)
               uint8_t tima;     // Timer counter                (0xFF05, 1)
               uint8_t tma;      // Timer modulo                 (0xFF06, 1)
               uint8_t tac;      // Timer control                (0xFF07, 1)
               uint8_t pad1[7];  // Padding                      (0xFF08-0xFF0E, 7)
               uint8_t ifr;      // Interrupt flag               (0xFF0F, 1)

               // Sound mode 1
               uint8_t nr10;     // Sweep                        (0xFF10, 1)
               uint8_t nr11;     // Length / wave pattern duty   (0xFF11, 1)
               uint8_t nr12;     // Envelope                     (0xFF12, 1)
               uint8_t nr13;     // Frequency lo                 (0xFF13, 1)
               uint8_t nr14;     // Frequency hi                 (0xFF14, 1)
               uint8_t pad2[1];  // Padding                      (0xFF15, 1)

               // Sound mode 2
               uint8_t nr21;     // Length / wave pattern duty   (0xFF16, 1)
               uint8_t nr22;     // Envelope                     (0xFF17, 1)
               uint8_t nr23;     // Frequency lo                 (0xFF18, 1)
               uint8_t nr24;     // Frequency hi                 (0xFF19, 1)

               // Sound mode 3
               uint8_t nr30;     // On / off                     (0xFF1A, 1)
               uint8_t nr31;     // Length                       (0xFF1B, 1)
               uint8_t nr32;     // Output level                 (0xFF1C, 1)
               uint8_t nr33;     // Frequency lo                 (0xFF1D, 1)
               uint8_t nr34;     // Frequency hi                 (0xFF1E, 1)
               uint8_t pad3[1];  // Padding                      (0xFF1F, 1)

               // Sound mode 4
               uint8_t nr41;     // Length                       (0xFF20, 1)
               uint8_t nr42;     // Envelope                     (0xFF21, 1)
               uint8_t nr43;     // Polynomial counter           (0xFF22, 1)
               uint8_t nr44;     // Counter/consecutive; initial (0xFF23, 1)

               // Sound general
               uint8_t nr50;     // Channel control/on-off/vol.  (0xFF24, 1)
               uint8_t nr51;     // Output terminal selection    (0xFF25, 1)
               uint8_t nr52;     // On / off                     (0xFF26, 1)
               uint8_t pad4[9];  // Padding                      (0xFF27-0xFF2F, 9)
               uint8_t wave[16]; // Wave pattern RAM             (0xFF30-0xFF3F, 16)

               // LCD
               uint8_t lcdc;     // LCD control                  (0xFF40, 1)
               uint8_t stat;     // LCD status                   (0xFF41, 1)
               uint8_t scy;      // Scroll y                     (0xFF42, 1)
               uint8_t scx;      // Scroll x                     (0xFF43, 1)
               uint8_t ly;       // LCDC y-coordinate            (0xFF44, 1)
               uint8_t lyc;      // LY compare                   (0xFF45, 1)
               uint8_t dma;      // DMA transfer & start address (0xFF46, 1)
               uint8_t bgp;      // BG & window palette data     (0xFF47, 1)
               uint8_t obp0;     // Object palette 0 data        (0xFF48, 1)
               uint8_t obp1;     // Object palette 1 data        (0xFF49, 1)
               uint8_t wy;       // Window y position            (0xFF4A, 1)
               uint8_t wx;       // Window x position            (0xFF4B, 1)
               uint8_t pad5[4];  // Padding                      (0xFF4C-0xFF4F, 4)

               uint8_t boot;     // Executing bootstrap          (0xFF50, 1)
               uint8_t pad6[47]; // Padding                      (0xFF51-0xFF7F, 47)
            };
         };

         uint8_t ramh[0x7F];     // High RAM area                (0xFF80-0xFFFE, 127)
         uint8_t ie;             // Interrupt enable register    (0xFFFF, 1)
      };
   };

   uint8_t& operator[](size_t index) {
      ASSERT(index < 0x10000);

      if (index >= 0xE000 && index < 0xFE00) {
         // Mirror of working ram
         index -= 0x2000;
      }

      return raw[index];
   }

   const uint8_t& operator[](size_t index) const {
      return (*const_cast<Memory*>(this))[index];
   }
};

} // namespace GBC

#endif

