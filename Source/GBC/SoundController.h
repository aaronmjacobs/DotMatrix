#ifndef GBC_SOUND_CONTROLLER_H
#define GBC_SOUND_CONTROLLER_H

#include <cstdint>
#include <vector>

namespace GBC {

class SoundController {
public:
   SoundController(class Memory& memory);

   void tick(uint64_t cycles);

   std::vector<uint8_t> getAudioData() const;

private:
   class Memory& mem;
};

} // namespace GBC

#endif
