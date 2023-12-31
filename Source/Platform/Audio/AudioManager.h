#pragma once

#if !DM_WITH_AUDIO
#   error "Including AudioManager header, but DM_WITH_AUDIO is not set!"
#endif // !DM_WITH_AUDIO

#include "GameBoy/SoundController.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

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
   void queue(std::span<DotMatrix::AudioSample> audioData);

   void setPitch(float pitch);

private:
   std::unique_ptr<ALCdevice, std::function<void(ALCdevice*)>> device;
   std::unique_ptr<ALCcontext, std::function<void(ALCcontext*)>> context;
   ALuint source = 0;
   std::array<ALuint, 3> buffers = {};
   float currentPitch = -1.0f;
};
