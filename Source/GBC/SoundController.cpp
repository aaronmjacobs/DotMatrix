#include "Log.h"

#include "GBC/Memory.h"
#include "GBC/SoundController.h"

#include <cmath>

namespace GBC {

SoundController::SoundController(Memory& memory)
   : mem(memory) {
}

void SoundController::tick(uint64_t cycles) {
}

std::vector<uint8_t> SoundController::getAudioData() const {
   // sample rate of 44100 * frame time of 1/60 second = 735 samples per tick
   // That comes out to 367.5 8-bit values at 4 bits per sample
   return std::vector<uint8_t>(368, 0x88);
}

} // namespace GBC
