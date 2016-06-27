#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>

namespace GBC {

union Memory {
   uint8_t raw[0x10000];    // Memory map                   (0x0000-0xFFFF, 64k)

   struct {
      uint8_t romb[0x4000]; // Permanently-mapped ROM bank  (0x0000-0x3FFF, 16k)
      uint8_t roms[0x4000]; // Switchable ROM bank          (0x4000-0x7FFF, 16k)
      uint8_t vram[0x2000]; // Video RAM                    (0x8000-0x9FFF, 8k)
      uint8_t eram[0x2000]; // Switchable external RAM bank (0xA000-0xBFFF, 8k)
      uint8_t ram0[0x1000]; // Working RAM bank 0           (0xC000-0xCFFF, 4k)
      uint8_t ram1[0x1000]; // Working RAM bank 1           (0xD000-0xDFFF, 4k)
      uint8_t ramm[0x1E00]; // Mirror of working ram        (0xE000-0xFDFF, 7680)
      uint8_t sat[0x100];   // Sprite attribute table       (0xFE00-0xFEFF, 256)
      uint8_t io[0x80];     // I/O device mappings          (0xFF00-0xFF7F, 128)
      uint8_t ramh[0x7F];   // High RAM area                (0xFF80-0xFFFE, 127)
      uint8_t ier;          // Interrupt enable register    (0xFFFF, 1)
   };
};

} // namespace GBC

#endif

