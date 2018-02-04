#include "AudioManager.h"

#include "FancyAssert.h"
#include "Log.h"

#include <AL/al.h>
#include <AL/alc.h>

namespace {

const ALsizei kAudioFrequency = 44100;
const size_t kBufferSize = 1470;

void deleteDevice(ALCdevice* device) {
   if (device) {
      alcCloseDevice(device);
   }
}

void deleteContext(ALCcontext* context) {
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

void checkError(const char* location) {
   ALCenum error = alGetError();
   if (error != AL_NO_ERROR) {
      LOG_ERROR("OpenAL error while " << location << ": " << errorString(error));
   }
}

std::vector<uint8_t> mono4ToMono8(const std::vector<uint8_t>& mono4) {
   std::vector<uint8_t> mono8(mono4.size() * 2);

   for (size_t i = 0; i < mono4.size(); ++i) {
      uint8_t value = mono4[i];
      uint8_t first = (value & 0xF0) >> 4;
      uint8_t second = (value & 0x0F);

      mono8[2 * i + 0] = first << 4;
      mono8[2 * i + 1] = second << 4;
   }

   return mono8;
}

} // namespace

AudioManager::AudioManager()
   : device(alcOpenDevice(nullptr), deleteDevice), source(0) {
   if (!device) {
      LOG_ERROR_MSG_BOX("Unable to open audio device");
      return;
   }

   context = UPtr<ALCcontext, std::function<void(ALCcontext*)>>(alcCreateContext(device.get(), nullptr), deleteContext);
   if (!context) {
      LOG_ERROR_MSG_BOX("Unable to create audio context");
      return;
   }

   if (!alcMakeContextCurrent(context.get())) {
      LOG_ERROR_MSG_BOX("Unable to make audio context current");
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

   alGenBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
   RUN_DEBUG(checkError("generating buffers");)

   std::array<uint8_t, kBufferSize> silence;
   silence.fill(128);
   for (ALuint buffer : buffers) {
      alBufferData(buffer, AL_FORMAT_MONO8, silence.data(), static_cast<ALsizei>(silence.size()), kAudioFrequency);
      RUN_DEBUG(checkError("setting buffer data");)
   }

   alSourceQueueBuffers(source, static_cast<ALsizei>(buffers.size()), buffers.data());
   RUN_DEBUG(checkError("queueing buffers");)

   alSourcePlay(source);
   RUN_DEBUG(checkError("playing source");)
}

AudioManager::~AudioManager() {
   if (alIsSource(source)) {
      alDeleteSources(1, &source);
      alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
   }

   alcMakeContextCurrent(nullptr);

   // Destroy in order
   context = nullptr;
   device = nullptr;
}

void AudioManager::queue(const std::vector<uint8_t>& audioData) {
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

   std::vector<uint8_t> audioData8 = mono4ToMono8(audioData);
   alBufferData(buffer, AL_FORMAT_MONO8, audioData8.data(), static_cast<ALsizei>(audioData8.size()), kAudioFrequency);
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
