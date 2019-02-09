#pragma once

#include "Core/Pointers.h"

#include "GBC/SoundController.h"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

struct ALCcontext_struct;
typedef struct ALCcontext_struct ALCcontext;
struct ALCdevice_struct;
typedef struct ALCdevice_struct ALCdevice;
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

private:
   UPtr<ALCdevice, std::function<void(ALCdevice*)>> device;
   UPtr<ALCcontext, std::function<void(ALCcontext*)>> context;
   ALuint source;
   std::array<ALuint, 3> buffers;
};
