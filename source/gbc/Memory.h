#ifndef GBC_MEMORY_H
#define GBC_MEMORY_H

#include <array>
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

namespace Boot {

enum Enum : uint8_t {
   kBooting = 0,
   kNotBooting = 1
};

};

class Memory {
public:
   template<size_t size>
   using ByteArray = std::array<uint8_t, size>;

   Memory();

   union {
      ByteArray<0x10000> raw;       // Direct access                 (0x0000-0xFFFF, 64k)

      struct {
         ByteArray<0x4000> romb;    // Permanently-mapped ROM bank   (0x0000-0x3FFF, 16k)
         ByteArray<0x4000> roms;    // Switchable ROM bank           (0x4000-0x7FFF, 16k)
         ByteArray<0x2000> vram;    // Video RAM                     (0x8000-0x9FFF, 8k)
         ByteArray<0x2000> eram;    // Switchable external RAM bank  (0xA000-0xBFFF, 8k)
         ByteArray<0x1000> ram0;    // Working RAM bank 0            (0xC000-0xCFFF, 4k)
         ByteArray<0x1000> ram1;    // Working RAM bank 1            (0xD000-0xDFFF, 4k)
         ByteArray<0x1E00> ramm;    // Mirror of working ram         (0xE000-0xFDFF, 7680)
         ByteArray<0x0100> sat;     // Sprite attribute table        (0xFE00-0xFEFF, 256)

         union {                    // I/O device mappings           (0xFF00-0xFF7F, 128)
            ByteArray<0x0080> io;   // Direct access                 (0xFF00-0xFF7F, 128)

            struct {
               // General
               uint8_t p1;          // Joy pad / system info         (0xFF00, 1)
               uint8_t sb;          // Serial transfer data          (0xFF01, 1)
               uint8_t sc;          // Serial IO control             (0xFF02, 1)
               ByteArray<1> pad0;   // Padding                       (0xFF03, 1)
               uint8_t div;         // Divider register              (0xFF04, 1)
               uint8_t tima;        // Timer counter                 (0xFF05, 1)
               uint8_t tma;         // Timer modulo                  (0xFF06, 1)
               uint8_t tac;         // Timer control                 (0xFF07, 1)
               ByteArray<7> pad1;   // Padding                       (0xFF08-0xFF0E, 7)
               uint8_t ifr;         // Interrupt flag                (0xFF0F, 1)

               // Sound mode 1
               uint8_t nr10;        // Sweep                         (0xFF10, 1)
               uint8_t nr11;        // Length / wave pattern duty    (0xFF11, 1)
               uint8_t nr12;        // Envelope                      (0xFF12, 1)
               uint8_t nr13;        // Frequency lo                  (0xFF13, 1)
               uint8_t nr14;        // Frequency hi                  (0xFF14, 1)
               ByteArray<1> pad2;   // Padding                       (0xFF15, 1)

               // Sound mode 2
               uint8_t nr21;        // Length / wave pattern duty    (0xFF16, 1)
               uint8_t nr22;        // Envelope                      (0xFF17, 1)
               uint8_t nr23;        // Frequency lo                  (0xFF18, 1)
               uint8_t nr24;        // Frequency hi                  (0xFF19, 1)

               // Sound mode 3
               uint8_t nr30;        // On / off                      (0xFF1A, 1)
               uint8_t nr31;        // Length                        (0xFF1B, 1)
               uint8_t nr32;        // Output level                  (0xFF1C, 1)
               uint8_t nr33;        // Frequency lo                  (0xFF1D, 1)
               uint8_t nr34;        // Frequency hi                  (0xFF1E, 1)
               ByteArray<1> pad3;   // Padding                       (0xFF1F, 1)

               // Sound mode 4
               uint8_t nr41;        // Length                        (0xFF20, 1)
               uint8_t nr42;        // Envelope                      (0xFF21, 1)
               uint8_t nr43;        // Polynomial counter            (0xFF22, 1)
               uint8_t nr44;        // Counter/consecutive; initial  (0xFF23, 1)

               // Sound general
               uint8_t nr50;        // Channel control/on-off/vol.   (0xFF24, 1)
               uint8_t nr51;        // Output terminal selection     (0xFF25, 1)
               uint8_t nr52;        // On / off                      (0xFF26, 1)
               ByteArray<9> pad4;   // Padding                       (0xFF27-0xFF2F, 9)
               ByteArray<16> wave;  // Wave pattern RAM              (0xFF30-0xFF3F, 16)

               // LCD
               uint8_t lcdc;        // LCD control                   (0xFF40, 1)
               uint8_t stat;        // LCD status                    (0xFF41, 1)
               uint8_t scy;         // Scroll y                      (0xFF42, 1)
               uint8_t scx;         // Scroll x                      (0xFF43, 1)
               uint8_t ly;          // LCDC y-coordinate             (0xFF44, 1)
               uint8_t lyc;         // LY compare                    (0xFF45, 1)
               uint8_t dma;         // DMA transfer & start address  (0xFF46, 1)
               uint8_t bgp;         // BG & window palette data      (0xFF47, 1)
               uint8_t obp0;        // Object palette 0 data         (0xFF48, 1)
               uint8_t obp1;        // Object palette 1 data         (0xFF49, 1)
               uint8_t wy;          // Window y position             (0xFF4A, 1)
               uint8_t wx;          // Window x position             (0xFF4B, 1)
               ByteArray<4> pad5;   // Padding                       (0xFF4C-0xFF4F, 4)

               // Bootstrap
               uint8_t boot;        // Executing bootstrap           (0xFF50, 1)
               ByteArray<47> pad6;  // Padding                       (0xFF51-0xFF7F, 47)
            };
         };

         ByteArray<0x007F> ramh;    // High RAM area                 (0xFF80-0xFFFE, 127)
         uint8_t ie;                // Interrupt enable register     (0xFFFF, 1)
      };
   };

   const uint8_t* getAddr(uint16_t address) const;

   uint8_t get(uint16_t address) const {
      return *getAddr(address);
   }

   void set(uint16_t address, uint8_t val);

   void setCartridge(class Cartridge* cartridge) {
      cart = cartridge;
   }

private:
   void executeDMATransfer();

   class Cartridge* cart;
};

} // namespace GBC

#endif
