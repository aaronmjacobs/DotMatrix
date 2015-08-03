#include "AudioManager.h"

#include "FancyAssert.h"
#include "LogHelper.h"
#include "gbc/Audio.h"

#include <AL/al.h>
#include <AL/alc.h>

namespace {

const ALsizei kAudioFrequency = 44100;

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

   alGenBuffers(buffers.size(), buffers.data());
   RUN_DEBUG(checkError("generating buffers");)

   std::array<uint8_t, GBC::kAudioBufferSize> silence;
   silence.fill(128);
   for (ALuint buffer : buffers) {
      alBufferData(buffer, AL_FORMAT_MONO8, silence.data(), silence.size(), kAudioFrequency);
      RUN_DEBUG(checkError("setting buffer data");)
   }

   alSourceQueueBuffers(source, buffers.size(), buffers.data());
   RUN_DEBUG(checkError("queueing buffers");)

   alSourcePlay(source);
   RUN_DEBUG(checkError("playing source");)
}

AudioManager::~AudioManager() {
   if (alIsSource(source)) {
      alDeleteSources(1, &source);
      alDeleteBuffers(buffers.size(), buffers.data());
   }

   alcMakeContextCurrent(nullptr);

   // Destroy in order
   context = nullptr;
   device = nullptr;
}

void AudioManager::queue(const uint8_t *data, size_t size) {
   ALint numProcessed;
   alGetSourcei(source, AL_BUFFERS_PROCESSED, &numProcessed);
   RUN_DEBUG(checkError("querying number of buffers processed");)

   if (numProcessed < 1) {
      // No room for more audio data
      return;
   }

   ALuint buffer;
   alSourceUnqueueBuffers(source, 1, &buffer);
   RUN_DEBUG(checkError("unqueueing buffer");)

   alBufferData(buffer, AL_FORMAT_MONO8, data, size, kAudioFrequency);
   RUN_DEBUG(checkError("setting buffer data");)

   alSourceQueueBuffers(source, 1, &buffer);
   RUN_DEBUG(checkError("queueing buffer");)

   ALint state;
   alGetSourcei(source, AL_SOURCE_STATE, &state);
   RUN_DEBUG(checkError("getting source state");)

   if (state != AL_PLAYING) {
      alSourcePlay(source);
      RUN_DEBUG(checkError("playing source");)
   }
}
