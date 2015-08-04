#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "Types.h"

#include <array>
#include <cstdint>
#include <functional>

struct ALCcontext_struct;
typedef struct ALCcontext_struct ALCcontext;
struct ALCdevice_struct;
typedef struct ALCdevice_struct ALCdevice;
typedef unsigned int ALuint;

class AudioManager {
private:
   UPtr<ALCdevice, std::function<void(ALCdevice*)>> device;
   UPtr<ALCcontext, std::function<void(ALCcontext*)>> context;
   ALuint source;
   std::array<ALuint, 2> buffers;

public:
   AudioManager();

   virtual ~AudioManager();

   void queue(const uint8_t *data, size_t size);
};

#endif
