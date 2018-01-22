#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "Pointers.h"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

struct ALCcontext_struct;
typedef struct ALCcontext_struct ALCcontext;
struct ALCdevice_struct;
typedef struct ALCdevice_struct ALCdevice;
typedef unsigned int ALuint;

class AudioManager {
public:
   AudioManager();
   ~AudioManager();

   void queue(const std::vector<uint8_t>& audioData);

private:
   UPtr<ALCdevice, std::function<void(ALCdevice*)>> device;
   UPtr<ALCcontext, std::function<void(ALCcontext*)>> context;
   ALuint source;
   std::array<ALuint, 2> buffers;
};

#endif
