#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>

namespace GBC {

union Memory {
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
            uint8_t p1;       // Joy pad / system info        (0xFF00, 1)
            uint8_t sb;       // Serial transfer data         (0xFF01, 1)
            uint8_t sc;       // IO Control                   (0xFF02, 1)
            uint8_t pad0[1];  // Padding                      (0xFF03, 1)
            uint8_t div;      // Divider register             (0xFF04, 1)
            uint8_t tima;     // Timer counter                (0xFF05, 1)
            uint8_t tma;      // Timer Modulo                 (0xFF06, 1)
            uint8_t tac;      // Timer Control                (0xFF07, 1)
            uint8_t pad1[7];  // Padding                      (0xFF08-0xFF0E, 7)
            uint8_t iff;      // Interrupt Flag               (0xFF0F, 1)
            uint8_t pad2[112];// Padding                      (0xFF10-0xFF7F, 112)
         };
      };

      uint8_t ramh[0x7F];     // High RAM area                (0xFF80-0xFFFE, 127)
      uint8_t ie;             // Interrupt enable register    (0xFFFF, 1)
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

