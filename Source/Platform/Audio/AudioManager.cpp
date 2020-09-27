#include "Platform/Audio/AudioManager.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <boxer/boxer.h>
#include <DotMatrixCore/Core/Assert.h>
#include <DotMatrixCore/Core/Log.h>
#include <DotMatrixCore/GameBoy/SoundController.h>

namespace
{
#if DM_DEBUG
   const char* alErrorString(ALenum error)
   {
      switch (error)
      {
      case AL_NO_ERROR:
         return "No error";
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

   const char* alcErrorString(ALCenum error)
   {
      switch (error)
      {
      case ALC_NO_ERROR:
         return "No error";
      case ALC_INVALID_DEVICE:
         return "Invalid device handle";
      case ALC_INVALID_CONTEXT:
         return "Invalid context handle";
      case ALC_INVALID_ENUM:
         return "Invalid enum parameter passed to an ALC call";
      case ALC_INVALID_VALUE:
         return "Invalid value parameter passed to an ALC call";
      case ALC_OUT_OF_MEMORY:
         return "Out of memory";
      default:
         return "Invalid error";
      }
   }
#endif // DM_DEBUG

   void checkAlError(const char* location)
   {
#if DM_DEBUG
      ALenum error = alGetError();
      DM_ASSERT(error == AL_NO_ERROR, "OpenAL error while %s: %s", location, alErrorString(error));
#endif // DM_DEBUG
   }

   void checkAlcError(ALCdevice* device, const char* location)
   {
#if DM_DEBUG
      ALCenum error = alcGetError(device);
      DM_ASSERT(error == ALC_NO_ERROR, "OpenAL context error while %s: %s", location, alcErrorString(error));
#endif // DM_DEBUG
   }

   void deleteDevice(ALCdevice* device)
   {
      if (device)
      {
         alcCloseDevice(device);
         checkAlcError(device, "closing device");
      }
   }

   void deleteContext(ALCcontext* context)
   {
      if (context)
      {
         ALCdevice* device = alcGetContextsDevice(context);

         alcDestroyContext(context);
         checkAlcError(device, "destroying context");
      }
   }
}

AudioManager::AudioManager()
   : device(alcOpenDevice(nullptr), deleteDevice)
{
   checkAlcError(device.get(), "opening device");

   if (!device)
   {
      boxer::show("Unable to open audio device", "Error", boxer::Style::Error);
      return;
   }

   context = std::unique_ptr<ALCcontext, std::function<void(ALCcontext*)>>(alcCreateContext(device.get(), nullptr), deleteContext);
   checkAlcError(device.get(), "creating context");

   if (!context)
   {
      boxer::show("Unable to create audio context", "Error", boxer::Style::Error);
      return;
   }

   if (!alcMakeContextCurrent(context.get()))
   {
      boxer::show("Unable to make audio context current", "Error", boxer::Style::Error);
      return;
   }
   checkAlcError(device.get(), "making audio context current");

   alGenSources(1, &source);
   checkAlError("generating audio source");

   setPitch(1.0f);
   alSourcef(source, AL_GAIN, 0.5f); // Full volume is pretty loud since our samples use the full range of values, so we lower it here
   checkAlError("setting source gain");
   alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
   checkAlError("setting source position");
   alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
   checkAlError("setting source velocity");
   alSourcei(source, AL_LOOPING, AL_FALSE);
   checkAlError("disabling source looping");

   alGenBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
   checkAlError("generating buffers");

   DotMatrix::AudioSample silenceSample;
   for (ALuint buffer : buffers)
   {
      alBufferData(buffer, AL_FORMAT_STEREO16, &silenceSample, static_cast<ALsizei>(sizeof(DotMatrix::AudioSample)), static_cast<ALsizei>(DotMatrix::SoundController::kSampleRate));
      checkAlError("setting buffer data");
   }

   alSourceQueueBuffers(source, static_cast<ALsizei>(buffers.size()), buffers.data());
   checkAlError("queueing buffers");

   alSourcePlay(source);
   checkAlError("playing source");
}

AudioManager::~AudioManager()
{
   if (alIsSource(source))
   {
      alDeleteSources(1, &source);
      checkAlError("deleting source");

      alDeleteBuffers(static_cast<ALsizei>(buffers.size()), buffers.data());
      checkAlError("deleting buffers");
   }

   alcMakeContextCurrent(nullptr);
   checkAlcError(device.get(), "making no context current");

   // Destroy in order
   context = nullptr;
   device = nullptr;
}

bool AudioManager::canQueue() const
{
   if (!isValid())
   {
      return false;
   }

   ALint numProcessed = 0;
   alGetSourcei(source, AL_BUFFERS_PROCESSED, &numProcessed);
   checkAlError("querying number of buffers processed");

   return numProcessed > 0;
}

void AudioManager::queue(const std::vector<DotMatrix::AudioSample>& audioData)
{
   DM_ASSERT(!audioData.empty());
   DM_ASSERT(canQueue());

   ALuint buffer = 0;
   alSourceUnqueueBuffers(source, 1, &buffer);
   checkAlError("unqueueing buffer");

   alBufferData(buffer, AL_FORMAT_STEREO16, audioData.data(), static_cast<ALsizei>(audioData.size() * sizeof(DotMatrix::AudioSample)), static_cast<ALsizei>(DotMatrix::SoundController::kSampleRate));
   checkAlError("setting buffer data");

   alSourceQueueBuffers(source, 1, &buffer);
   checkAlError("queueing buffer");

   ALint state = 0;
   alGetSourcei(source, AL_SOURCE_STATE, &state);
   checkAlError("getting source state");

   if (state != AL_PLAYING)
   {
      alSourcePlay(source);
      checkAlError("playing source");
   }
}

void AudioManager::setPitch(float pitch)
{
   alSourcef(source, AL_PITCH, pitch);
   checkAlError("setting source pitch");
}
