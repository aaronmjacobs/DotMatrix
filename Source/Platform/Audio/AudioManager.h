#pragma once

#if !GBC_WITH_AUDIO
#   error "Including AudioManager header, but GBC_WITH_AUDIO is not set!"
#endif // !GBC_WITH_AUDIO

#include "Core/Pointers.h"

#include "GBC/SoundController.h"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

struct ALCcontext;
struct ALCdevice;
typedef unsigned int ALuint;

class AudioManager
{
public:
   AudioManager();
   ~AudioManager();

   bool isValid() const
   {
      return device.get() != nullptr && context.get() != nullptr;
   }

   bool canQueue() const;
   void queue(const std::vector<GBC::AudioSample>& audioData);

   void setPitch(float pitch);

private:
   UPtr<ALCdevice, std::function<void(ALCdevice*)>> device;
   UPtr<ALCcontext, std::function<void(ALCcontext*)>> context;
   ALuint source = 0;
   std::array<ALuint, 3> buffers = {};
};
