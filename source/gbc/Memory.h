#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>

namespace GBC {

union Memory {
   uint8_t raw[1024 * 64]; // Memory map
   struct {
      uint8_t romb[1024 * 16];    // Permanently-mapped ROM bank
      uint8_t roms[1024 * 16];    // Switchable ROM bank
      uint8_t vram[1024 * 8];     // Video RAM
      uint8_t eram[1024 * 16];    // Switchable external RAM bank
      uint8_t ram0[1024 * 4];     // Working RAM bank 0
      uint8_t ram1[1024 * 4];     // Working RAM bank 1
      uint8_t sat[256];           // Sprite attribute table
      uint8_t io[128];            // I/O device mappings
      uint8_t ramh[127];          // High RAM area
      uint8_t ier;                // Interrupt enable register
   };
};

} // namespace GBC

#endif

