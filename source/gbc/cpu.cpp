#include <stdint.h>
uint8_t a;          // accumulator register
uint8_t f;          // status register
union {             // general purpose registers
    struct {
        uint8_t b;
        uint8_t c;
        uint8_t d;
        uint8_t e;
        uint8_t h;
        uint8_t l;
    }
    struct {
        uint16_t bc;
        uint16_t de;
        uint16_t hl;
    }
}
uint16_t pc;        // program counter
uint16_t sp;        // stack pointer
union {
    uint8_t memory[1024 * 64]; // Memory map
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

