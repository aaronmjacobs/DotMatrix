#pragma once

#include "DotMatrixCore/GameBoy/CPU.h"

#include <array>
#include <cstdint>
#include <vector>

namespace DotMatrix
{

class SoundController;
class SquareWaveChannel;

struct AudioSample
{
   int16_t left = 0;
   int16_t right = 0;
};

class SoundChannel
{
public:
   bool isEnabled() const
   {
      return enabled;
   }

   void disable()
   {
      enabled = false;
   }

protected:
   void trigger()
   {
      enabled = true;
   }

private:
   bool enabled = true;
};

template<typename Owner>
class SoundTimer
{
public:
   SoundTimer(Owner& owningObject)
      : owner(owningObject)
   {
   }

   void machineCycle()
   {
      uint32_t cycles = CPU::kClockCyclesPerMachineCycle;

      if (period != 0)
      {
         while (cycles >= counter)
         {
            cycles -= counter;
            owner.clock();
            counter = period;
         }

         counter -= cycles;
      }
   }

   void setPeriod(uint32_t newPeriod)
   {
      period = newPeriod;
   }

private:
   Owner& owner;
   uint32_t period = 0;
   uint32_t counter = 0;
};

class DutyUnit
{
public:
   void clock()
   {
      high = kDutyMasks[index][counter];
      counter = (counter + 1) % 8;
   }

   void reset()
   {
      counter = 0;
   }

   uint8_t readNrx1() const
   {
      DM_ASSERT((index & 0xFC) == 0x00);
      return index << 6;
   }

   void writeNrx1(uint8_t value)
   {
      index = (value >> 6) & 0x03;
   }

   bool isHigh() const
   {
      return high;
   }

private:
   static constexpr const std::array<std::array<bool, 8>, 4> kDutyMasks =
   {{
      { false, false, false, false, false, false, false, true },
      { true, false, false, false, false, false, false, true },
      { true, false, false, false, false, true, true, true },
      { false, true, true, true, true, true, true, false }
   }};

   uint8_t counter = 0;
   uint8_t index = 0;
   bool high = false;
};

class LengthUnit
{
public:
   LengthUnit(SoundChannel& owningSoundChannel, uint16_t maxCounterValue)
      : owner(owningSoundChannel)
      , maxCounter(maxCounterValue)
   {
      DM_ASSERT(maxCounter == 64 || maxCounter == 256);
   }

   void clock()
   {
      if (enabled && counter > 0)
      {
         --counter;

         if (counter == 0)
         {
            owner.disable();
         }
      }
   }

   void trigger()
   {
      if (counter == 0)
      {
         counter = maxCounter;
      }
   }

   uint8_t readNrx1() const
   {
      DM_ASSERT((counterLoad & ~(maxCounter - 1)) == 0x00);
      return counterLoad;
   }

   uint8_t readNrx4() const
   {
      return enabled ? 0x40 : 0x00;
   }

   void writeNrx1(uint8_t value)
   {
      uint8_t mask = maxCounter - 1;
      counterLoad = value & mask;
      counter = maxCounter - counterLoad;
   }

   void writeNrx4(uint8_t value)
   {
      enabled = (value & 0x40) != 0x00;
   }

private:
   SoundChannel& owner;
   const uint16_t maxCounter;
   uint16_t counter = 0;
   uint8_t counterLoad = 0;
   bool enabled = false;
};

class EnvelopeUnit
{
public:
   void clock();

   void trigger()
   {
      resetCounter();
      volume = volumeLoad;
      enabled = true;
   }

   bool isDacPowered() const
   {
      return dacPowered;
   }

   uint8_t readNrx2() const
   {
      uint8_t value = 0x00;

      DM_ASSERT((volumeLoad & 0xF0) == 0x00);
      value |= volumeLoad << 4;

      value |= addMode ? 0x08 : 0x00;

      DM_ASSERT((period & 0xF8) == 0x00);
      value |= period;

      return value;
   }

   void writeNrx2(uint8_t value)
   {
      volumeLoad = (value & 0xF0) >> 4;
      addMode = (value & 0x08) != 0x00;
      period = value & 0x07;
      dacPowered = (value & 0xF8) != 0x00;
   }

   uint8_t getVolume() const
   {
      return volume;
   }

private:
   void resetCounter()
   {
      // The volume envelope and sweep timers treat a period of 0 as 8
      counter = period == 0 ? 8 : period;
   }

   uint8_t period = 0;
   uint8_t counter = 8;
   uint8_t volume = 0;
   uint8_t volumeLoad = 0;
   bool addMode = false;
   bool enabled = true;
   bool dacPowered = false;
};

class SweepUnit
{
public:
   SweepUnit(SquareWaveChannel& owningSquareWaveChannel)
      : owner(owningSquareWaveChannel)
   {
   }

   void clock();

   void trigger()
   {
      // Sweep timer is reloaded
      resetCounter();

      updateEnabledState();

      if (shift != 0)
      {
         // Frequency calculation and the overflow check are performed immediately
         calculateNewFrequency();
      }
   }

   uint8_t readNrx0() const
   {
      uint8_t value = 0x00;

      DM_ASSERT((period & 0x08) == 0x00);
      value |= period << 4;

      value |= negate ? 0x08 : 0x00;

      DM_ASSERT((shift & 0xF8) == 0x00);
      value |= shift;

      return value;
   }

   void writeNrx0(uint8_t value)
   {
      period = (value >> 4) & 0x07;
      negate = (value & 0x08) != 0x00;
      shift = value & 0x07;
   }

   void setShadowFrequency(uint16_t newShadowFrequency)
   {
      DM_ASSERT(newShadowFrequency < 2048);
      shadowFrequency = newShadowFrequency;
   }

   void updateEnabledState()
   {
      enabled = period != 0 || shift != 0;
   }

   uint16_t calculateNewFrequency();

private:
   void resetCounter()
   {
      // The volume envelope and sweep timers treat a period of 0 as 8
      counter = period == 0 ? 8 : period;
   }

   SquareWaveChannel& owner;

   uint16_t shadowFrequency = 0;
   uint8_t period = 0;
   uint8_t counter = 8;
   uint8_t shift = 0;
   bool negate = false;
   bool enabled = false;
};

class WaveUnit
{
public:
   void clock()
   {
      position = (position + 1) % 32;
   }

   void trigger()
   {
      position = 0;
   }

   void reset()
   {
      position = 0;
   }

   bool isDacPowered() const
   {
      return dacPowered;
   }

   int8_t getCurrentAudioSample() const;

   uint8_t readNrx0() const
   {
      return dacPowered ? 0x80 : 0x00;
   }

   uint8_t readNrx2() const
   {
      DM_ASSERT((volumeCode & 0xFC) == 0x00);
      return volumeCode << 5;
   }

   uint8_t readWaveTableValue(uint8_t index) const
   {
      return waveTable[index];
   }

   void writeNrx0(uint8_t value)
   {
      dacPowered = (value & 0x80) != 0x00;
   }

   void writeNrx2(uint8_t value)
   {
      volumeCode = (value >> 5) & 0x03;
   }

   void writeWaveTableValue(uint8_t index, uint8_t value)
   {
      waveTable[index] = value;
   }

private:
   uint8_t position = 0;
   uint8_t volumeCode = 0;
   bool dacPowered = false;
   std::array<uint8_t, 16> waveTable = { 0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA }; // TODO Wave table initial values different on CGB
};

class LFSRUnit
{
public:
   void clock();

   void trigger()
   {
      lfsr = 0xFFFF;
   }

   uint8_t readNrx3() const
   {
      uint8_t value = 0x00;

      DM_ASSERT((clockShift & 0xF0) == 0x00);
      value |= clockShift << 4;

      value |= widthMode ? 0x08 : 0x00;

      DM_ASSERT((divisorCode & 0xF8) == 0x00);
      value |= divisorCode;

      return value;
   }

   void writeNrx3(uint8_t value)
   {
      clockShift = (value & 0xF0) >> 4;
      widthMode = (value & 0x08) != 0x00;
      divisorCode = value & 0x07;
   }

   bool isHigh() const
   {
      return (lfsr & 0x0001) == 0x00;
   }

   uint32_t calcTimerPeriod() const
   {
      static const std::array<uint8_t, 8> kDivisorValues =
      {{
            8, 16, 32, 48, 64, 80, 96, 112
      }};

      return kDivisorValues[divisorCode] << clockShift;
   }

private:
   uint8_t clockShift = 0;
   bool widthMode = false;
   uint8_t divisorCode = 0;
   uint16_t lfsr = 0xFFFF;
};

class SquareWaveChannel : public SoundChannel
{
public:
   SquareWaveChannel();

   bool isEnabled() const
   {
      return SoundChannel::isEnabled() && envelopeUnit.isDacPowered();
   }

   int8_t getCurrentAudioSample() const;

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   void machineCycle()
   {
      timer.machineCycle();
   }

   void clock()
   {
      dutyUnit.clock();
   }

   void lengthClock()
   {
      lengthUnit.clock();
   }

   void envelopeClock()
   {
      envelopeUnit.clock();
   }

   void sweepClock()
   {
      sweepUnit.clock();
   }

   void trigger()
   {
      SoundChannel::trigger();

      lengthUnit.trigger();
      envelopeUnit.trigger();
      sweepUnit.trigger();
   }

   void resetDutyUnit()
   {
      dutyUnit.reset();
   }

   void setFrequency(uint16_t newFrequency)
   {
      frequency = newFrequency;
      timer.setPeriod((2048 - frequency) * 4);
   }

private:
   SoundTimer<SquareWaveChannel> timer;
   uint16_t frequency = 0;

   DutyUnit dutyUnit;
   LengthUnit lengthUnit;
   EnvelopeUnit envelopeUnit;
   SweepUnit sweepUnit;
};

class WaveChannel : public SoundChannel
{
public:
   WaveChannel();

   bool isEnabled() const
   {
      return SoundChannel::isEnabled() && waveUnit.isDacPowered();
   }

   int8_t getCurrentAudioSample() const;

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   void machineCycle()
   {
      timer.machineCycle();
   }

   void clock()
   {
      waveUnit.clock();
   }

   void lengthClock()
   {
      lengthUnit.clock();
   }

   void trigger()
   {
      SoundChannel::trigger();

      waveUnit.trigger();
      lengthUnit.trigger();
   }

   void resetWaveUnit()
   {
      waveUnit.reset();
   }

private:
   void setFrequency(uint16_t newFrequency)
   {
      frequency = newFrequency;
      timer.setPeriod((2048 - frequency) * 2);
   }

   SoundTimer<WaveChannel> timer;
   uint16_t frequency = 0;

   WaveUnit waveUnit;
   LengthUnit lengthUnit;
};

class NoiseChannel : public SoundChannel
{
public:
   NoiseChannel();

   bool isEnabled() const
   {
      return SoundChannel::isEnabled() && envelopeUnit.isDacPowered();
   }

   int8_t getCurrentAudioSample() const;

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   void machineCycle()
   {
      timer.machineCycle();
   }

   void clock()
   {
      lfsrUnit.clock();
   }

   void lengthClock()
   {
      lengthUnit.clock();
   }

   void envelopeClock()
   {
      envelopeUnit.clock();
   }

   void trigger()
   {
      SoundChannel::trigger();

      lfsrUnit.trigger();
      lengthUnit.trigger();
      envelopeUnit.trigger();
   }

private:
   SoundTimer<NoiseChannel> timer;

   LFSRUnit lfsrUnit;
   LengthUnit lengthUnit;
   EnvelopeUnit envelopeUnit;
};

class FrameSequencer
{
public:
   FrameSequencer(SoundController& owningSoundController)
      : timer(*this)
      , owner(owningSoundController)
   {
      timer.setPeriod(CPU::kClockSpeed / 512);
   }

   void machineCycle()
   {
      timer.machineCycle();
   }

   void clock();

   void reset()
   {
      step = 0;
   }

private:
   SoundTimer<FrameSequencer> timer;
   SoundController& owner;
   uint8_t step = 0;
};

class Mixer
{
public:
   AudioSample mix(int8_t square1Sample, int8_t square2Sample, int8_t waveSample, int8_t noiseSample) const;

   uint8_t readNr50() const;
   uint8_t readNr51() const;

   void writeNr50(uint8_t value);
   void writeNr51(uint8_t value);

private:
   uint8_t leftVolume = 0x01;
   uint8_t rightVolume = 0x01;

   bool vinLeftEnabled = false;
   bool vinRightEnabled = false;

   bool square1LeftEnabled = false;
   bool square1RightEnabled = false;
   bool square2LeftEnabled = false;
   bool square2RightEnabled = false;
   bool waveLeftEnabled = false;
   bool waveRightEnabled = false;
   bool noiseLeftEnabled = false;
   bool noiseRightEnabled = false;
};

class SoundController
{
public:
   static const size_t kSampleRate = 65536;

   SoundController();

   void setGenerateAudioData(bool generateAudioData)
   {
      generateData = generateAudioData;

      if (!generateData)
      {
         cyclesSinceLastSample = 0;
         for (std::vector<AudioSample>& buffer : buffers)
         {
            buffer.clear();
         }
      }
   }

   const std::vector<AudioSample>& swapAudioBuffers()
   {
      activeBufferIndex = !activeBufferIndex;

      buffers[activeBufferIndex].clear();
#if DM_WITH_PER_CHANNEL_AUDIO_BUFFERS
      square1Buffers[activeBufferIndex].clear();
      square2Buffers[activeBufferIndex].clear();
      waveBuffers[activeBufferIndex].clear();
      noiseBuffers[activeBufferIndex].clear();
#endif // DM_WITH_PER_CHANNEL_AUDIO_BUFFERS

      return buffers[!activeBufferIndex];
   }

   void machineCycle();

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

private:
   friend class FrameSequencer;

   const std::vector<AudioSample>& getAudioData() const
   {
      return buffers[activeBufferIndex];
   }

#if DM_WITH_PER_CHANNEL_AUDIO_BUFFERS
   const std::vector<int8_t>& getSquare1Data() const
   {
      return square1Buffers[activeBufferIndex];
   }
   const std::vector<int8_t>& getSquare2Data() const
   {
      return square2Buffers[activeBufferIndex];
   }
   const std::vector<int8_t>& getWaveData() const
   {
      return waveBuffers[activeBufferIndex];
   }
   const std::vector<int8_t>& getNoiseData() const
   {
      return noiseBuffers[activeBufferIndex];
   }
#endif // DM_WITH_PER_CHANNEL_AUDIO_BUFFERS

   uint8_t readNr52() const;
   void writeNr52(uint8_t value);
   void setPowerEnabled(bool newPowerEnabled);
   void pushSample();

   void lengthClock()
   {
      squareWaveChannel1.lengthClock();
      squareWaveChannel2.lengthClock();
      waveChannel.lengthClock();
      noiseChannel.lengthClock();
   }

   void envelopeClock()
   {
      squareWaveChannel1.envelopeClock();
      squareWaveChannel2.envelopeClock();
      noiseChannel.envelopeClock();
   }

   void sweepClock()
   {
      squareWaveChannel1.sweepClock();
   }

   FrameSequencer frameSequencer;
   Mixer mixer;
   bool powerEnabled = false;

   SquareWaveChannel squareWaveChannel1;
   SquareWaveChannel squareWaveChannel2;
   WaveChannel waveChannel;
   NoiseChannel noiseChannel;

   bool generateData = false;
   uint8_t cyclesSinceLastSample = 0;
   std::size_t activeBufferIndex = 0;
   std::array<std::vector<AudioSample>, 2> buffers;

#if DM_WITH_PER_CHANNEL_AUDIO_BUFFERS
   std::array<std::vector<int8_t>, 2> square1Buffers;
   std::array<std::vector<int8_t>, 2> square2Buffers;
   std::array<std::vector<int8_t>, 2> waveBuffers;
   std::array<std::vector<int8_t>, 2> noiseBuffers;
#endif // DM_WITH_PER_CHANNEL_AUDIO_BUFFERS
};

} // namespace DotMatrix
