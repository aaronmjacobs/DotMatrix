#ifndef GBC_SOUND_CONTROLLER_H
#define GBC_SOUND_CONTROLLER_H

#include "GBC/CPU.h"

#include <cstdint>
#include <vector>

namespace GBC {

class SoundController;
class SquareWaveChannel;

struct AudioSample {
   int16_t left = 0;
   int16_t right = 0;
};

class SoundChannel {
public:
   SoundChannel() : enabled(true) {
   }

   void trigger() {
      enabled = true;
   }

   void disable() {
      enabled = false;
   }

   bool isEnabled() const {
      return enabled;
   }

private:
   bool enabled;
};

template<typename Owner>
class SoundTimer {
public:
   SoundTimer(Owner& owningObject) : owner(owningObject), period(0), counter(0) {
   }

   void tick(uint32_t cycles) {
      if (period != 0) {
         while (cycles >= counter) {
            cycles -= counter;
            owner.clock();
            counter = period;
         }

         counter -= cycles;
      }
   }

   void setPeriod(uint32_t newPeriod) {
      period = newPeriod;
   }

private:
   uint32_t period;
   uint32_t counter;
   Owner& owner;
};

class DutyUnit {
public:
   DutyUnit() : counter(0), index(0), high(false) {
   }

   void clock() {
      static const std::array<std::array<bool, 8>, 4> kDutyMasks = {{
         { false, false, false, false, false, false, false, true },
         { true, false, false, false, false, false, false, true },
         { true, false, false, false, false, true, true, true },
         { false, true, true, true, true, true, true, false }
      }};

      high = kDutyMasks[index][counter];
      counter = (counter + 1) % 8;
   }

   void reset() {
      counter = 0;
   }

   uint8_t readNrx1() const {
      ASSERT((index & 0xFC) == 0x00);
      return index << 6;
   }

   void writeNrx1(uint8_t value) {
      index = (value >> 6) & 0x03;
   }

   bool isHigh() const {
      return high;
   }

private:
   uint8_t counter;
   uint8_t index;
   bool high;
};

class LengthUnit {
public:
   LengthUnit(SoundChannel& owningSoundChannel, uint16_t maxCounterValue) : owner(owningSoundChannel), maxCounter(maxCounterValue), counter(0), counterLoad(0), enabled(false) {
      ASSERT(maxCounter == 64 || maxCounter == 256);
   }

   void clock() {
      if (enabled && counter > 0) {
         --counter;

         if (counter == 0) {
            owner.disable();
         }
      }
   }

   void trigger() {
      if (counter == 0) {
         counter = maxCounter;
      }
   }

   uint8_t readNrx1() const {
      ASSERT((counterLoad & ~(maxCounter - 1)) == 0x00);
      return counterLoad;
   }

   uint8_t readNrx4() const {
      return enabled ? 0x40 : 0x00;
   }

   void writeNrx1(uint8_t value) {
      uint8_t mask = maxCounter - 1;
      counterLoad = value & mask;
      counter = maxCounter - counterLoad;
   }

   void writeNrx4(uint8_t value) {
      enabled = (value & 0x40) != 0x00;
   }

private:
   SoundChannel& owner;
   const uint16_t maxCounter;
   uint16_t counter;
   uint8_t counterLoad;
   bool enabled;
};

class EnvelopeUnit {
public:
   EnvelopeUnit() : period(0), counter(0), volume(0), volumeLoad(0), addMode(false), enabled(true) {
   }

   void clock();

   void trigger() {
      counter = period == 0 ? 8 : period;
      volume = volumeLoad;
      enabled = true;
   }

   uint8_t readNrx2() const {
      uint8_t value = 0x00;

      ASSERT((volumeLoad & 0xF0) == 0x00);
      value |= volumeLoad << 4;

      value |= addMode ? 0x08 : 0x00;

      ASSERT((period & 0xF8) == 0x00);
      value |= period;

      return value;
   }

   void writeNrx2(uint8_t value) {
      volumeLoad = (value & 0xF0) >> 4;
      addMode = (value & 0x08) != 0x00;
      period = value & 0x07;
   }

   uint8_t getVolume() const {
      return volume;
   }

private:
   uint8_t period;
   uint8_t counter;
   uint8_t volume;
   uint8_t volumeLoad;
   bool addMode;
   bool enabled;
};

class SweepUnit {
public:
   SweepUnit(SquareWaveChannel& owningSquareWaveChannel) : owner(owningSquareWaveChannel), shadowFrequency(0), period(0), counter(0), shift(0), negate(false), enabled(false) {
   }

   void clock();

   void trigger() {
      // Sweep timer is reloaded
      counter = period == 0 ? 8 : period;

      updateEnabledState();

      if (shift != 0) {
         // Frequency calculation and the overflow check are performed immediately
         calculateNewFrequency();
      }
   }

   uint8_t readNrx0() const {
      uint8_t value = 0x00;

      ASSERT((period & 0x08) == 0x00);
      value |= period << 4;

      value |= negate ? 0x08 : 0x00;

      ASSERT((shift & 0xF8) == 0x00);
      value |= shift;

      return value;
   }

   void writeNrx0(uint8_t value) {
      period = (value >> 4) & 0x07;
      negate = (value & 0x08) != 0x00;
      shift = value & 0x07;
   }

   void setShadowFrequency(uint16_t newShadowFrequency) {
      ASSERT(newShadowFrequency < 2048);
      shadowFrequency = newShadowFrequency;
   }

   void updateEnabledState() {
      enabled = period != 0 || shift != 0;
   }

   uint16_t calculateNewFrequency();

private:
   SquareWaveChannel& owner;

   uint16_t shadowFrequency;
   uint8_t period;
   uint8_t counter;
   uint8_t shift;
   bool negate;
   bool enabled;
};

class WaveUnit {
public:
   // TODO Wave table initial values different on CGB
   WaveUnit() : position(0), volumeCode(0), dacPowered(false), waveTable{ 0x84, 0x40, 0x43, 0xAA, 0x2D, 0x78, 0x92, 0x3C, 0x60, 0x59, 0x59, 0xB0, 0x34, 0xB8, 0x2E, 0xDA }  {
   }

   void clock() {
      position = (position + 1) % 32;
   }

   void trigger() {
      position = 0;
   }

   void reset() {
      position = 0;
   }

   int8_t getCurrentAudioSample() const;

   bool isDacPowered() const {
      return dacPowered;
   }

   uint8_t readNrx0() const {
      return dacPowered ? 0x80 : 0x00;
   }

   uint8_t readNrx2() const {
      ASSERT((volumeCode & 0xFC) == 0x00);
      return volumeCode << 5;
   }

   uint8_t readWaveTableValue(uint8_t index) const {
      return waveTable[index];
   }

   void writeNrx0(uint8_t value) {
      dacPowered = (value & 0x80) != 0x00;
   }

   void writeNrx2(uint8_t value) {
      volumeCode = (value >> 5) & 0x03;
   }

   void writeWaveTableValue(uint8_t index, uint8_t value) {
      waveTable[index] = value;
   }

private:
   uint8_t position;
   uint8_t volumeCode;
   bool dacPowered;
   std::array<uint8_t, 16> waveTable;
};

class LFSRUnit {
public:
   LFSRUnit() : clockShift(0), widthMode(false), divisorCode(0), lfsr(0xFFFF) {
   }

   void clock();

   void trigger() {
      lfsr = 0xFFFF;
   }

   uint8_t readNrx3() const {
      uint8_t value = 0x00;

      ASSERT((clockShift & 0xF0) == 0x00);
      value |= clockShift << 4;

      value |= widthMode ? 0x08 : 0x00;

      ASSERT((divisorCode & 0xF8) == 0x00);
      value |= divisorCode;

      return value;
   }

   void writeNrx3(uint8_t value) {
      clockShift = (value & 0xF0) >> 4;
      widthMode = (value & 0x08) != 0x00;
      divisorCode = value & 0x07;
   }

   bool isHigh() const {
      return (lfsr & 0x0001) == 0x00;
   }

   uint32_t calcTimerPeriod() const {
      static const std::array<uint8_t, 8> kDivisorValues = {{
            8, 16, 32, 48, 64, 80, 96, 112
      }};

      return kDivisorValues[divisorCode] << clockShift;
   }

private:
   uint8_t clockShift;
   bool widthMode;
   uint8_t divisorCode;
   uint16_t lfsr;
};

class SquareWaveChannel : public SoundChannel {
public:
   SquareWaveChannel();

   int8_t getCurrentAudioSample() const;

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   void tick(uint32_t cycles) {
      timer.tick(cycles);
   }

   void clock() {
      dutyUnit.clock();
   }

   void lengthClock() {
      lengthUnit.clock();
   }

   void envelopeClock() {
      envelopeUnit.clock();
   }

   void sweepClock() {
      sweepUnit.clock();
   }

   void trigger() {
      SoundChannel::trigger();

      lengthUnit.trigger();
      envelopeUnit.trigger();
      sweepUnit.trigger();
   }

   void resetDutyUnit() {
      dutyUnit.reset();
   }

   void setFrequency(uint16_t newFrequency) {
      frequency = newFrequency;
      timer.setPeriod((2048 - frequency) * 4);
   }

private:
   SoundTimer<SquareWaveChannel> timer;
   uint16_t frequency;

   DutyUnit dutyUnit;
   LengthUnit lengthUnit;
   EnvelopeUnit envelopeUnit;
   SweepUnit sweepUnit;
};

class WaveChannel : public SoundChannel {
public:
   WaveChannel();

   int8_t getCurrentAudioSample() const;

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   void tick(uint32_t cycles) {
      timer.tick(cycles);
   }

   void clock() {
      waveUnit.clock();
   }

   void lengthClock() {
      lengthUnit.clock();
   }

   void trigger() {
      SoundChannel::trigger();

      waveUnit.trigger();
      lengthUnit.trigger();
   }

   void resetWaveUnit() {
      waveUnit.reset();
   }

private:
   void setFrequency(uint16_t newFrequency) {
      frequency = newFrequency;
      timer.setPeriod((2048 - frequency) * 2);
   }

   SoundTimer<WaveChannel> timer;
   uint16_t frequency;

   WaveUnit waveUnit;
   LengthUnit lengthUnit;
};

class NoiseChannel : public SoundChannel {
public:
   NoiseChannel();

   int8_t getCurrentAudioSample() const;

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   void tick(uint32_t cycles) {
      timer.tick(cycles);
   }

   void clock() {
      lfsrUnit.clock();
   }

   void lengthClock() {
      lengthUnit.clock();
   }

   void envelopeClock() {
      envelopeUnit.clock();
   }

   void trigger() {
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

class FrameSequencer {
public:
   FrameSequencer(SoundController& owningSoundController) : timer(*this), owner(owningSoundController), step(0) {
      timer.setPeriod(CPU::kClockSpeed / 512);
   }

   void tick(uint32_t cycles) {
      timer.tick(cycles);
   }

   void clock();

   void reset() {
      step = 0;
   }

private:
   SoundTimer<FrameSequencer> timer;
   SoundController& owner;
   uint8_t step;
};

class Mixer {
public:
   Mixer();

   AudioSample mix(int8_t square1Sample, int8_t square2Sample, int8_t waveSample, int8_t noiseSample) const;

   void writeNr50(uint8_t value);
   void writeNr51(uint8_t value);

private:
   uint8_t leftVolume;
   uint8_t rightVolume;

   bool vinLeftEnabled;
   bool vinRightEnabled;

   bool square1LeftEnabled;
   bool square1RightEnabled;
   bool square2LeftEnabled;
   bool square2RightEnabled;
   bool waveLeftEnabled;
   bool waveRightEnabled;
   bool noiseLeftEnabled;
   bool noiseRightEnabled;
};

class SoundController {
public:
   static const size_t kSampleRate = 44100;

   SoundController();

   void setGenerateAudioData(bool generateAudioData) {
      generateData = generateAudioData;

      if (!generateData) {
         cyclesSinceLastSample = 0;
         data.clear();
         dataNeedsClear = false;
      }
   }

   const std::vector<AudioSample>& getAudioData() {
      dataNeedsClear = true;
      return data;
   }

   void tick(uint64_t cycles);

   uint8_t read(uint16_t address) const;
   void write(uint16_t address, uint8_t value);

   void lengthClock() {
      squareWaveChannel1.lengthClock();
      squareWaveChannel2.lengthClock();
      waveChannel.lengthClock();
      noiseChannel.lengthClock();
   }

   void envelopeClock() {
      squareWaveChannel1.envelopeClock();
      squareWaveChannel2.envelopeClock();
      noiseChannel.envelopeClock();
   }

   void sweepClock() {
      squareWaveChannel1.sweepClock();
   }

private:
   void setPowerEnabled(bool newPowerEnabled);
   AudioSample getCurrentAudioSample() const;

   FrameSequencer frameSequencer;
   Mixer mixer;
   bool powerEnabled;

   SquareWaveChannel squareWaveChannel1;
   SquareWaveChannel squareWaveChannel2;
   WaveChannel waveChannel;
   NoiseChannel noiseChannel;

   bool generateData;
   uint64_t cyclesSinceLastSample;
   bool dataNeedsClear;
   std::vector<AudioSample> data;
};

} // namespace GBC

#endif
