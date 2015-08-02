#include "AudioManager.h"

#include "FancyAssert.h"
#include "LogHelper.h"

#include <AL/al.h>
#include <AL/alc.h>

namespace {

void deleteDevice(ALCdevice *device) {
   if (device) {
      alcCloseDevice(device);
   }
}

void deleteContext(ALCcontext *context) {
   if (context) {
      alcDestroyContext(context);
   }
}

const char* errorString(ALCenum error) {
   switch (error) {
      case AL_NO_ERROR:
         return "";
      case AL_INVALID_NAME:
         return "Invalid name parameter";
      case AL_INVALID_ENUM:
         return "Invalid parameter";
      case AL_INVALID_VALUE:
         return "Invalid enum parameter value";
      case AL_INVALID_OPERATION:
         return "Illegal call";
      case AL_OUT_OF_MEMORY:
         return "Unable to allocate memory";
      default:
         return "Invalid error";
   }
}

void checkError(const char *location) {
   ALCenum error = alGetError();
   if (error != AL_NO_ERROR) {
      LOG_ERROR("OpenAL error while " << location << ": " << errorString(error));
   }
}

} // namespace

AudioManager::AudioManager()
   : device(alcOpenDevice(nullptr), deleteDevice), source(0) {
   if (!device) {
      LOG_ERROR("Unable to open audio device");
      return;
   }

   context = UPtr<ALCcontext, std::function<void(ALCcontext*)>>(alcCreateContext(device.get(), nullptr), deleteContext);
   if (!context) {
      LOG_ERROR("Unable to create audio context");
      return;
   }

   if (!alcMakeContextCurrent(context.get())) {
      LOG_ERROR("Unable to make audio context current");
      return;
   }
   RUN_DEBUG(checkError("making audio context current");)

   alGenSources(1, &source);
   RUN_DEBUG(checkError("generating audio source");)

   alSourcef(source, AL_PITCH, 1.0f);
   RUN_DEBUG(checkError("setting source pitch");)
   alSourcef(source, AL_GAIN, 1.0f);
   RUN_DEBUG(checkError("setting source gain");)
   alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
   RUN_DEBUG(checkError("setting source position");)
   alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
   RUN_DEBUG(checkError("setting source velocity");)
   alSourcei(source, AL_LOOPING, AL_FALSE);
   RUN_DEBUG(checkError("disabling source looping");)

   alGenBuffers(1, &buffer);
   RUN_DEBUG(checkError("generating buffer");)

   alSourcei(source, AL_BUFFER, buffer);
   RUN_DEBUG(checkError("binding buffer to source");)

   alSourcePlay(source);
   RUN_DEBUG(checkError("playing source");)
}

AudioManager::~AudioManager() {
   if (alIsSource(source)) {
      alDeleteSources(1, &source);
   }

   if (alIsBuffer(buffer)) {
      alDeleteBuffers(1, &buffer);
   }

   alcMakeContextCurrent(nullptr);

   // Destroy in order
   context = nullptr;
   device = nullptr;
}

void AudioManager::bufferData(const uint8_t *data, size_t size) {
   alBufferData(buffer, AL_FORMAT_MONO8, data, size, 44100);
   RUN_DEBUG(checkError("setting buffer data");)
}
