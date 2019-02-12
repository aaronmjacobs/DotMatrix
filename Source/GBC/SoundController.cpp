#include "Core/Log.h"

#include "GBC/CPU.h"
#include "GBC/SoundController.h"

#include <cmath>

namespace GBC
{

void EnvelopeUnit::clock()
{
   if (counter == 0)
   {
      // The volume envelope and sweep timers treat a period of 0 as 8
      counter = period == 0 ? 8 : period;

      if (period != 0 && enabled)
      {
         uint8_t newVolume = addMode ? volume + 1 : volume - 1;
         if (newVolume < 16)
         {
            volume = newVolume;
         }
         else
         {
            enabled = false;
         }
      }
   }
   else
   {
      --counter;
   }
}

void SweepUnit::clock()
{
   if (counter == 0)
   {
      counter = period == 0 ? 8 : period; // TODO Correct?

      if (enabled && period != 0)
      {
         uint16_t newFrequency = calculateNewFrequency();

         if (newFrequency < 2048 && shift != 0)
         {
            shadowFrequency = newFrequency;
            owner.setFrequency(newFrequency);

            calculateNewFrequency();
         }
      }
   }
   else
   {
      --counter;
   }
}

uint16_t SweepUnit::calculateNewFrequency()
{
   uint16_t newFrequency = shadowFrequency >> shift;

   if (negate)
   {
      newFrequency = shadowFrequency - newFrequency;
   }
   else
   {
      newFrequency = shadowFrequency + newFrequency;
   }

   if (newFrequency >= 2048)
   {
      owner.disable();
   }

   return newFrequency;
}

int8_t WaveUnit::getCurrentAudioSample() const
{
   uint8_t sampleIndex = position / 2;
   uint8_t value = waveTable[sampleIndex];
   uint8_t sample = position % 2 == 0 ? ((value & 0xF0) >> 4) : (value & 0x0F);

   ASSERT(volumeCode < 4);
   uint8_t shift = volumeCode == 0 ? 4 : volumeCode - 1;
   uint8_t shiftedSample = sample >> shift;

   return (shiftedSample - 0x08);
}

void LFSRUnit::clock()
{
   uint8_t bit0 = lfsr & 0x01;
   lfsr >>= 1;
   uint8_t bit1 = lfsr & 0x01;
   uint8_t xorResult = bit0 ^ bit1;

   lfsr = (lfsr & 0b1011111111111111) | (xorResult << 14);
   if (widthMode)
   {
      lfsr = (lfsr & 0b1111111110111111) | (xorResult << 6);
   }
}

SquareWaveChannel::SquareWaveChannel()
   : timer(*this)
   , frequency(0)
   , lengthUnit(*this, 64)
   , sweepUnit(*this)
{
}

int8_t SquareWaveChannel::getCurrentAudioSample() const
{
   int8_t sample = 0;

   if (isEnabled())
   {
      uint8_t volume = envelopeUnit.getVolume();
      sample = dutyUnit.isHigh() ? volume : -volume;
   }

   return sample;
}

uint8_t SquareWaveChannel::read(uint16_t address) const
{
   uint8_t value = 0x00;

   switch (address)
   {
   case 0xFF10:
      value |= 0x80;
      value |= sweepUnit.readNrx0();
      break;
   case 0xFF11:
   case 0xFF16:
      value |= 0x3F;
      value |= dutyUnit.readNrx1();
      value |= lengthUnit.readNrx1();
      break;
   case 0xFF12:
   case 0xFF17:
      value |= envelopeUnit.readNrx2();
      break;
   case 0xFF13:
   case 0xFF18:
      value |= 0xFF;
      value |= frequency & 0x00FF;
      break;
   case 0xFF14:
   case 0xFF19:
      value |= 0xBF;
      ASSERT((frequency & 0xF800) == 0x00);
      value |= frequency >> 8;
      value |= lengthUnit.readNrx4();

      // TODO trigger?
      break;
   default:
      //ASSERT(false);
      value = Memory::kInvalidAddressByte;
      break;
   }

   return value;
}

void SquareWaveChannel::write(uint16_t address, uint8_t value)
{
   switch (address)
   {
   case 0xFF10:
      // Sweep period, negate, shift
      sweepUnit.writeNrx0(value);
      break;
   case 0xFF11:
   case 0xFF16:
      // Duty, Length load (64-L)
      dutyUnit.writeNrx1(value);
      lengthUnit.writeNrx1(value);
      break;
   case 0xFF12:
   case 0xFF17:
      // Starting volume, Envelope add mode, period
      envelopeUnit.writeNrx2(value);
      break;
   case 0xFF13:
   case 0xFF18:
      // Frequency LSB
      setFrequency((frequency & 0x0700) | value);
      break;
   case 0xFF14:
   case 0xFF19:
      setFrequency(((value << 8) & 0x0700) | (frequency & 0x00FF));
      lengthUnit.writeNrx4(value);

      // Trigger, Length enable, Frequency MSB
      if ((value & 0x80) != 0x00)
      {
         sweepUnit.setShadowFrequency(frequency);
         trigger();
      }
      break;
   default:
      //ASSERT(false);
      break;
   }
}

WaveChannel::WaveChannel()
   : timer(*this)
   , frequency(0)
   , lengthUnit(*this, 256)
{
}

int8_t WaveChannel::getCurrentAudioSample() const
{
   int8_t sample = 0;

   if (isEnabled() && waveUnit.isDacPowered())
   {
      sample = waveUnit.getCurrentAudioSample();
   }

   return sample;
}

uint8_t WaveChannel::read(uint16_t address) const
{
   uint8_t value = 0x00;

   switch (address)
   {
   case 0xFF1A:
      value |= 0x7F;
      value |= waveUnit.readNrx0();
      break;
   case 0xFF1B:
      value |= 0xFF;
      value |= lengthUnit.readNrx1();
      break;
   case 0xFF1C:
      value |= 0x9F;
      value |= waveUnit.readNrx2();
      break;
   case 0xFF1D:
      value |= 0xFF;
      value |= frequency & 0x00FF;
      break;
   case 0xFF1E:
      value |= 0xBF;
      ASSERT((frequency & 0xF800) == 0x00);
      value |= frequency >> 8;
      value |= lengthUnit.readNrx4();

      // TODO trigger?
      break;
   default:
      if (address >= 0xFF30 && address <= 0xFF3F)
      {
         uint8_t index = static_cast<uint8_t>(address - 0xFF30);
         value |= waveUnit.readWaveTableValue(index);
      }
      else
      {
         //ASSERT(false);
         value = Memory::kInvalidAddressByte;
      }
      break;
   }

   return value;
}

void WaveChannel::write(uint16_t address, uint8_t value)
{
   switch (address)
   {
   case 0xFF1A:
      // DAC power
      waveUnit.writeNrx0(value);
      break;
   case 0xFF1B:
      // Length load (256-L)
      lengthUnit.writeNrx1(value);
      break;
   case 0xFF1C:
      // Volume code (00=0%, 01=100%, 10=50%, 11=25%)
      waveUnit.writeNrx2(value);
      break;
   case 0xFF1D:
      // Frequency LSB
      setFrequency((frequency & 0x0700) | value);
      break;
   case 0xFF1E:
      // Trigger, Length enable, Frequency MSB
      setFrequency(((value << 8) & 0x0700) | (frequency & 0x00FF));
      lengthUnit.writeNrx4(value);

      if ((value & 0x80) != 0x00)
      {
         trigger();
      }
      break;
   default:
      if (address >= 0xFF30 && address <= 0xFF3F)
      {
         uint8_t index = static_cast<uint8_t>(address - 0xFF30);
         waveUnit.writeWaveTableValue(index, value);
      }
      else
      {
         //ASSERT(false);
      }
      break;
   }
}

NoiseChannel::NoiseChannel()
   : timer(*this)
   , lengthUnit(*this, 64)
{
}

int8_t NoiseChannel::getCurrentAudioSample() const
{
   int8_t sample = 0;

   if (isEnabled())
   {
      uint8_t volume = envelopeUnit.getVolume();
      sample = lfsrUnit.isHigh() ? volume : -volume;
   }

   return sample;
}

uint8_t NoiseChannel::read(uint16_t address) const
{
   uint8_t value = 0x00;

   switch (address)
   {
   case 0xFF20:
      value |= 0xFF;
      value |= lengthUnit.readNrx1();
      break;
   case 0xFF21:
      value |= envelopeUnit.readNrx2();
      break;
   case 0xFF22:
      value |= lfsrUnit.readNrx3();
      break;
   case 0xFF23:
      value |= 0xBF;
      value |= lengthUnit.readNrx4();
      break;
   default:
      //ASSERT(false);
      value = Memory::kInvalidAddressByte;
      break;
   }

   return value;
}

void NoiseChannel::write(uint16_t address, uint8_t value)
{
   switch (address)
   {
   case 0xFF20:
      // Length load (64-L)
      lengthUnit.writeNrx1(value);
      break;
   case 0xFF21:
      // Starting volume, Envelope add mode, period
      envelopeUnit.writeNrx2(value);
      break;
   case 0xFF22:
      // Clock shift, Width mode of LFSR, Divisor code
      lfsrUnit.writeNrx3(value);
      timer.setPeriod(lfsrUnit.calcTimerPeriod());
      break;
   case 0xFF23:
      // Trigger, Length enable
      lengthUnit.writeNrx4(value);

      if ((value & 0x80) != 0x00)
      {
         trigger();
      }
      break;
   default:
      //ASSERT(false);
      break;
   }
}

void FrameSequencer::clock()
{
   switch (step)
   {
   case 0:
      owner.lengthClock();
      break;
   case 1:
      break;
   case 2:
      owner.lengthClock();
      owner.sweepClock();
      break;
   case 3:
      break;
   case 4:
      owner.lengthClock();
      break;
   case 5:
      break;
   case 6:
      owner.lengthClock();
      owner.sweepClock();
      break;
   case 7:
      owner.envelopeClock();
      break;
   }

   step = (step + 1) % 8;
}

Mixer::Mixer()
   : leftVolume(0x01)
   , rightVolume(0x01)
   , vinLeftEnabled(false)
   , vinRightEnabled(false)
   , square1LeftEnabled(false)
   , square1RightEnabled(false)
   , square2LeftEnabled(false)
   , square2RightEnabled(false)
   , waveLeftEnabled(false)
   , waveRightEnabled(false)
   , noiseLeftEnabled(false)
   , noiseRightEnabled(false)
{
}

AudioSample Mixer::mix(int8_t square1Sample, int8_t square2Sample, int8_t waveSample, int8_t noiseSample) const
{
   // All samples should be in the range [-16, 16)
   ASSERT(square1Sample >= -16 && square1Sample < 16
      && square2Sample >= -16 && square2Sample < 16
      && waveSample >= -16 && waveSample < 16
      && noiseSample >= -16 && noiseSample < 16);

   int16_t square1LeftSample = square1LeftEnabled ? square1Sample * leftVolume : 0;
   int16_t square1RightSample = square1RightEnabled ? square1Sample * rightVolume : 0;

   int16_t square2LeftSample = square2LeftEnabled ? square2Sample * leftVolume : 0;
   int16_t square2RightSample = square2RightEnabled ? square2Sample * rightVolume : 0;

   int16_t waveLeftSample = waveLeftEnabled ? waveSample * leftVolume : 0;
   int16_t waveRightSample = waveRightEnabled ? waveSample * rightVolume : 0;

   int16_t noiseLeftSample = noiseLeftEnabled ? noiseSample * leftVolume : 0;
   int16_t noiseRightSample = noiseRightEnabled ? noiseSample * rightVolume : 0;

   // Each of the 4 samples uses a max of 5 bits (-16 = 0b00011111)
   // Volume multiplication has a max value of 8 (causing a max shift of 3 bits) producing a total usage of 8 bits
   // Adding all 4 samples uses a max of 10 bits (value can double each time, meaning two shifts)
   // That gives us a total of 6 bits left over, which we can shift into to maximize volume
   AudioSample sample;
   sample.left = (square1LeftSample + square2LeftSample + waveLeftSample + noiseLeftSample) << 6;
   sample.right = (square1RightSample + square2RightSample + waveRightSample + noiseRightSample) << 6;
   ASSERT(sample.left <= INT16_MAX && sample.left >= INT16_MIN
      && sample.right <= INT16_MAX && sample.right >= INT16_MIN);

   return sample;
}

uint8_t Mixer::readNr50() const
{
   uint8_t value = 0x00;

   ASSERT(leftVolume > 0 && ((leftVolume - 1) & 0x08) == 0x00);
   value |= (leftVolume - 1) << 4;

   ASSERT(rightVolume > 0 && ((rightVolume - 1) & 0x08) == 0x00);
   value |= (rightVolume - 1);

   value |= vinLeftEnabled ? 0x80 : 0x00;
   value |= vinRightEnabled ? 0x08 : 0x00;

   return value;
}

uint8_t Mixer::readNr51() const
{
   uint8_t value = 0x00;

   value |= square1LeftEnabled ? 0x10 : 0x00;
   value |= square1RightEnabled ? 0x01 : 0x00;
   value |= square2LeftEnabled ? 0x20 : 0x00;
   value |= square2RightEnabled ? 0x02 : 0x00;
   value |= waveLeftEnabled ? 0x40 : 0x00;
   value |= waveRightEnabled ? 0x04 : 0x00;
   value |= noiseLeftEnabled ? 0x80 : 0x00;
   value |= noiseRightEnabled ? 0x08 : 0x00;

   return value;
}

void Mixer::writeNr50(uint8_t value)
{
   leftVolume = ((value >> 4) & 0x07) + 1;
   rightVolume = (value & 0x07) + 1;

   vinLeftEnabled = (value & 0x80) != 0x00;
   vinRightEnabled = (value & 0x08) != 0x00;
}

void Mixer::writeNr51(uint8_t value)
{
   square1LeftEnabled = (value & 0x10) != 0x00;
   square1RightEnabled = (value & 0x01) != 0x00;
   square2LeftEnabled = (value & 0x20) != 0x00;
   square2RightEnabled = (value & 0x02) != 0x00;
   waveLeftEnabled = (value & 0x40) != 0x00;
   waveRightEnabled = (value & 0x04) != 0x00;
   noiseLeftEnabled = (value & 0x80) != 0x00;
   noiseRightEnabled = (value & 0x08) != 0x00;
}

SoundController::SoundController()
   : frameSequencer(*this)
   , powerEnabled(false)
   , generateData(false)
   , cyclesSinceLastSample(0)
   , activeBufferIndex(0)
{
   // Try to guess a reasonable amount of data to reserve
   // For now, 1/30th of a second's worth
   for (std::vector<AudioSample>& buffer : buffers)
   {
      buffer.reserve(kSampleRate / 30);
   }
}

void SoundController::machineCycle()
{
   static const uint64_t kCyclesPerSample = CPU::kClockSpeed / kSampleRate;
   //static_assert(CPU::kClockSpeed % kSampleRate == 0, "Sample rate does not divide evenly into the CPU clock speed!");

   frameSequencer.machineCycle();

   squareWaveChannel1.machineCycle();
   squareWaveChannel2.machineCycle();
   waveChannel.machineCycle();
   noiseChannel.machineCycle();

   if (generateData)
   {
      cyclesSinceLastSample += CPU::kClockCyclesPerMachineCycle;
      while (cyclesSinceLastSample >= kCyclesPerSample)
      {
         cyclesSinceLastSample -= kCyclesPerSample;

         AudioSample sample = getCurrentAudioSample();
         buffers[activeBufferIndex].push_back(sample);
      }
   }
}

uint8_t SoundController::read(uint16_t address) const
{
   uint8_t value = Memory::kInvalidAddressByte;

   if (address >= 0xFF10 && address <= 0xFF14)
   {
      // Channel 1
      value = squareWaveChannel1.read(address);
   }
   else if (address >= 0xFF15 && address <= 0xFF19)
   {
      // Channel 2
      value = squareWaveChannel2.read(address);
   }
   else if ((address >= 0xFF1A && address <= 0xFF1E) || (address >= 0xFF30 && address <= 0xFF3F))
   {
      // Channel 3
      value = waveChannel.read(address);
   }
   else if (address >= 0xFF1F && address <= 0xFF23)
   {
      // Channel 4
      value = noiseChannel.read(address);
   }
   else
   {
      // Control / Status
      switch (address)
      {
      case 0xFF24:
         value = mixer.readNr50();
         break;
      case 0xFF25:
         value = mixer.readNr51();
         break;
      case 0xFF26:
         value = readNr52();
         break;
      default:
         //ASSERT(false, "Trying to read value at invalid address from sound controller: %s", hex(address).c_str());
         break;
      }
   }

   return value;
}

void SoundController::write(uint16_t address, uint8_t value)
{
   if (!powerEnabled && address != 0xFF26)
   {
      // TODO Except length counters?
      return;
   }

   if (address >= 0xFF10 && address <= 0xFF14)
   {
      // Channel 1
      squareWaveChannel1.write(address, value);
   }
   else if (address >= 0xFF15 && address <= 0xFF19)
   {
      // Channel 2
      squareWaveChannel2.write(address, value);
   }
   else if ((address >= 0xFF1A && address <= 0xFF1E) || (address >= 0xFF30 && address <= 0xFF3F))
   {
      // Channel 3
      waveChannel.write(address, value);
   }
   else if (address >= 0xFF1F && address <= 0xFF23)
   {
      // Channel 4
      noiseChannel.write(address, value);
   }
   else
   {
      // Control / Status
      switch (address)
      {
      case 0xFF24:
         mixer.writeNr50(value);
         break;
      case 0xFF25:
         mixer.writeNr51(value);
         break;
      case 0xFF26:
         writeNr52(value);
         break;
      default:
         //ASSERT(false, "Trying to write value at invalid address from sound controller: %s at %s", hex(value).c_str(), hex(address).c_str());
         break;
      }
   }
}

uint8_t SoundController::readNr52() const
{
   uint8_t value = powerEnabled ? 0x80 : 0x00;

   value |= squareWaveChannel1.isEnabled() ? 0x01 : 0x00;
   value |= squareWaveChannel2.isEnabled() ? 0x02 : 0x00;
   value |= waveChannel.isEnabled() ? 0x03 : 0x00;
   value |= noiseChannel.isEnabled() ? 0x04 : 0x00;

   return value;
}

void SoundController::writeNr52(uint8_t value)
{
   setPowerEnabled((value & 0x80) != 0x00);
}

void SoundController::setPowerEnabled(bool newPowerEnabled)
{
   if (powerEnabled && !newPowerEnabled)
   {
      // Write 0x00 to all memory
      // TODO Except length counters?
      for (uint16_t address = 0xFF10; address < 0xFF26; ++address)
      {
         write(address, 0x00);
      }
   }
   else if (!powerEnabled && newPowerEnabled)
   {
      frameSequencer.reset();
      squareWaveChannel1.resetDutyUnit();
      squareWaveChannel2.resetDutyUnit();
      waveChannel.resetWaveUnit();
   }

   powerEnabled = newPowerEnabled;
}

AudioSample SoundController::getCurrentAudioSample() const
{
   int8_t square1Sample = squareWaveChannel1.getCurrentAudioSample();
   int8_t square2Sample = squareWaveChannel2.getCurrentAudioSample();
   int8_t waveSample = waveChannel.getCurrentAudioSample();
   int8_t noiseSample = noiseChannel.getCurrentAudioSample();

   return mixer.mix(square1Sample, square2Sample, waveSample, noiseSample);
}

} // namespace GBC
