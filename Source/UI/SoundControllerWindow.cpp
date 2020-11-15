#include "UI/UI.h"

#include "UI/BeforeCoreIncludes.inl"
#  include "Core/Math.h"
#  include "GameBoy/SoundController.h"
#include "UI/AfterCoreIncludes.inl"

#include <imgui.h>

#include <cmath>
#include <limits>
#include <sstream>
#include <string>

namespace DotMatrix
{

namespace
{
   const int kNumPlottedSamples = SoundController::kSampleRate / 4;

   float audioSampleGetter(void* data, int index)
   {
      std::vector<float>* floatSamples = reinterpret_cast<std::vector<float>*>(data);

      return (*floatSamples)[index];
   }

   void updateSamples(std::vector<float>& samples, int& offset, const std::vector<int8_t>& audioData)
   {
      for (int8_t audioSample : audioData)
      {
         samples[offset] = static_cast<float>(audioSample);
         offset = (offset + 1) % kNumPlottedSamples;
      }
   }

   std::string getPitch(uint32_t timerPeriod)
   {
      static const uint32_t kA4 = 440;
      static const uint32_t kC0 = Math::round<uint32_t>(kA4 * std::pow(2, -4.75));
      static const std::array<const char*, 12> kNames = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

      if (timerPeriod == 0)
      {
         return "";
      }

      double frequency = CPU::kClockSpeed / static_cast<double>(timerPeriod);
      uint32_t halfSteps = Math::round<uint32_t>(12 * std::log2(frequency / kC0));
      uint32_t octave = halfSteps / 12;
      uint32_t pitch = halfSteps % 12;

      std::stringstream ss;
      ss << kNames[pitch] << " " << octave;
      return ss.str();
   }
}

void UI::renderSoundControllerWindow(SoundController& soundController) const
{
   ImGui::SetNextWindowPos(ImVec2(5.0f, 322.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(570.0f, 451.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Sound Controller");

   if (ImGui::CollapsingHeader("Output", ImGuiTreeNodeFlags_DefaultOpen))
   {
      bool powerEnabled = soundController.powerEnabled;
      ImGui::Checkbox("Power enabled", &powerEnabled);
      if (powerEnabled != soundController.powerEnabled)
      {
         soundController.write(0xFF26, powerEnabled ? 0x80 : 0x00);
      }

      static std::vector<float> leftSamples(kNumPlottedSamples);
      static std::vector<float> rightSamples(kNumPlottedSamples);
      static int offset = 0;

      const std::vector<AudioSample>& audioData = soundController.getAudioData();
      for (const AudioSample& audioSample : audioData)
      {
         leftSamples[offset] = static_cast<float>(audioSample.left);
         rightSamples[offset] = static_cast<float>(audioSample.right);
         offset = (offset + 1) % kNumPlottedSamples;
      }

      ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 50.0f);
      ImGui::PlotLines("Left", audioSampleGetter, &leftSamples, kNumPlottedSamples, offset, nullptr, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), ImVec2(0.0f, 100.0f));
      ImGui::PlotLines("Right", audioSampleGetter, &rightSamples, kNumPlottedSamples, offset, nullptr, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max(), ImVec2(0.0f, 100.0f));
      ImGui::PopItemWidth();
   }

   if (ImGui::CollapsingHeader("Mixer"))
   {
      Mixer& mixer = soundController.mixer;
      uint8_t volumeMin = 0x01;
      uint8_t volumeMax = 0x08;

      ImGui::Columns(2, "mixer");

      ImGui::Checkbox("Vin left enabled", &mixer.vinLeftEnabled);
      ImGui::Checkbox("Square 1 left enabled", &mixer.square1LeftEnabled);
      ImGui::Checkbox("Square 2 left enabled", &mixer.square2LeftEnabled);
      ImGui::Checkbox("Wave left enabled", &mixer.waveLeftEnabled);
      ImGui::Checkbox("Noise left enabled", &mixer.noiseLeftEnabled);
      ImGui::SliderScalar("Left volume", ImGuiDataType_U8, &mixer.leftVolume, &volumeMin, &volumeMax, "0x%02X");
      ImGui::NextColumn();

      ImGui::Checkbox("Vin right enabled", &mixer.vinRightEnabled);
      ImGui::Checkbox("Square 1 right enabled", &mixer.square1RightEnabled);
      ImGui::Checkbox("Square 2 right enabled", &mixer.square2RightEnabled);
      ImGui::Checkbox("Wave right enabled", &mixer.waveRightEnabled);
      ImGui::Checkbox("Noise right enabled", &mixer.noiseRightEnabled);
      ImGui::SliderScalar("Right volume", ImGuiDataType_U8, &mixer.rightVolume, &volumeMin, &volumeMax, "0x%02X");
      ImGui::NextColumn();

      ImGui::Columns(1);
   }

   static const uint32_t kWaveMaxPeriod = 8192;

   if (ImGui::CollapsingHeader("Square Wave Channel 1"))
   {
      ImGui::PushID("Square Wave Channel 1");

      SquareWaveChannel& squareWaveChannel1 = soundController.squareWaveChannel1;

      static std::vector<float> samples(kNumPlottedSamples);
      static int offset = 0;
      updateSamples(samples, offset, soundController.getSquare1Data());

      renderSoundChannel(squareWaveChannel1, samples, offset);
      renderSoundTimer(squareWaveChannel1.timer, kWaveMaxPeriod, true);
      renderDutyUnit(squareWaveChannel1.dutyUnit);
      renderLengthUnit(squareWaveChannel1.lengthUnit);
      renderEnvelopeUnit(squareWaveChannel1.envelopeUnit);
      renderSweepUnit(squareWaveChannel1.sweepUnit);

      ImGui::PopID();
   }

   if (ImGui::CollapsingHeader("Square Wave Channel 2"))
   {
      ImGui::PushID("Square Wave Channel 2");

      SquareWaveChannel& squareWaveChannel2 = soundController.squareWaveChannel2;

      static std::vector<float> samples(kNumPlottedSamples);
      static int offset = 0;
      updateSamples(samples, offset, soundController.getSquare2Data());

      renderSoundChannel(squareWaveChannel2, samples, offset);
      renderSoundTimer(squareWaveChannel2.timer, kWaveMaxPeriod, true);
      renderDutyUnit(squareWaveChannel2.dutyUnit);
      renderLengthUnit(squareWaveChannel2.lengthUnit);
      renderEnvelopeUnit(squareWaveChannel2.envelopeUnit);

      ImGui::PopID();
   }

   if (ImGui::CollapsingHeader("Wave Channel"))
   {
      ImGui::PushID("Wave Channel");

      WaveChannel& waveChannel = soundController.waveChannel;

      static std::vector<float> samples(kNumPlottedSamples);
      static int offset = 0;
      updateSamples(samples, offset, soundController.getWaveData());

      renderSoundChannel(waveChannel, samples, offset);
      renderSoundTimer(waveChannel.timer, kWaveMaxPeriod, true);
      renderLengthUnit(waveChannel.lengthUnit);
      renderWaveUnit(waveChannel.waveUnit);

      ImGui::PopID();
   }

   if (ImGui::CollapsingHeader("Noise Channel"))
   {
      ImGui::PushID("Noise Channel");

      static const uint32_t kMaxNoisePeriod = 112 << 0x0F;

      NoiseChannel& noiseChannel = soundController.noiseChannel;

      static std::vector<float> samples(kNumPlottedSamples);
      static int offset = 0;
      updateSamples(samples, offset, soundController.getNoiseData());

      renderSoundChannel(noiseChannel, samples, offset);
      renderSoundTimer(noiseChannel.timer, kMaxNoisePeriod, false);
      renderLengthUnit(noiseChannel.lengthUnit);
      renderEnvelopeUnit(noiseChannel.envelopeUnit);
      renderLFSRUnit(noiseChannel.lfsrUnit);

      ImGui::PopID();
   }

   ImGui::End();
}

void UI::renderSoundChannel(SoundChannel& soundChannel, std::vector<float>& samples, int offset) const
{
   if (ImGui::TreeNode("Channel"))
   {
      ImGui::Checkbox("Enabled", &soundChannel.enabled);

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50.0f);
      ImGui::PlotLines("Output", audioSampleGetter, &samples, kNumPlottedSamples, offset, nullptr, -15.0f, 15.0f, ImVec2(0.0f, 100.0f));

      ImGui::TreePop();
   }
}

template<typename T>
void UI::renderSoundTimer(SoundTimer<T>& soundTimer, uint32_t maxPeriod, bool displayNote) const
{
   if (ImGui::TreeNode("Timer"))
   {
      ImGui::SliderScalar("Counter", ImGuiDataType_U32, &soundTimer.counter, &kZero, &soundTimer.period);
      ImGui::SliderScalar("Period", ImGuiDataType_U32, &soundTimer.period, &kZero, &maxPeriod);

      if (displayNote)
      {
         std::string pitch = getPitch(soundTimer.period);
         ImGui::Text("Pitch: %s", pitch.c_str());
      }

      ImGui::TreePop();
   }
}

void UI::renderDutyUnit(DutyUnit& dutyUnit) const
{
   if (ImGui::TreeNode("Duty unit"))
   {
      ImGui::Checkbox("High", &dutyUnit.high);

      uint8_t maxCounter = 7;
      ImGui::SliderScalar("Counter", ImGuiDataType_U8, &dutyUnit.counter, &kZero, &maxCounter);

      static const auto kHistogramGetter = [](void* data, int counter)
      {
         DutyUnit* dutyUnit = reinterpret_cast<DutyUnit*>(data);
         return DutyUnit::kDutyMasks[dutyUnit->index][counter] ? 1.0f : 0.0f;
      };

      ImGui::PlotHistogram("Duty", kHistogramGetter, &dutyUnit, maxCounter + 1);

      uint8_t maxIndex = 3;
      ImGui::SliderScalar("Index", ImGuiDataType_U8, &dutyUnit.index, &kZero, &maxIndex);

      ImGui::TreePop();
   }
}

void UI::renderLengthUnit(LengthUnit& lengthUnit) const
{
   if (ImGui::TreeNode("Length unit"))
   {
      ImGui::Checkbox("Enabled", &lengthUnit.enabled);
      ImGui::SliderScalar("Counter", ImGuiDataType_U16, &lengthUnit.counter, &kZero, &lengthUnit.maxCounter);
      ImGui::SliderScalar("Counter load", ImGuiDataType_U8, &lengthUnit.counterLoad, &kZero, &lengthUnit.maxCounter);

      ImGui::TreePop();
   }
}

void UI::renderEnvelopeUnit(EnvelopeUnit& envelopeUnit) const
{
   if (ImGui::TreeNode("Envelope unit"))
   {
      ImGui::Checkbox("DAC powered", &envelopeUnit.dacPowered);
      ImGui::Checkbox("Enabled", &envelopeUnit.enabled);
      ImGui::Checkbox("Add mode", &envelopeUnit.addMode);

      uint8_t maxCounter = 8;
      ImGui::SliderScalar("Counter", ImGuiDataType_U8, &envelopeUnit.counter, &kZero, &maxCounter);

      uint8_t maxPeriod = 0x07;
      ImGui::SliderScalar("Period", ImGuiDataType_U8, &envelopeUnit.period, &kZero, &maxPeriod);

      uint8_t maxVolume = 0x0F;
      ImGui::SliderScalar("Volume", ImGuiDataType_U8, &envelopeUnit.volume, &kZero, &maxVolume);
      ImGui::SliderScalar("Volume load", ImGuiDataType_U8, &envelopeUnit.volumeLoad, &kZero, &maxVolume);

      ImGui::TreePop();
   }
}

void UI::renderSweepUnit(SweepUnit& sweepUnit) const
{
   if (ImGui::TreeNode("Sweep unit"))
   {
      ImGui::Checkbox("Enabled", &sweepUnit.enabled);
      ImGui::Checkbox("Negate", &sweepUnit.negate);

      uint16_t maxFrequency = 0x07FF;
      ImGui::SliderScalar("Shadow frequency", ImGuiDataType_U16, &sweepUnit.shadowFrequency, &kZero, &maxFrequency);

      uint8_t maxCounter = 8;
      ImGui::SliderScalar("Counter", ImGuiDataType_U8, &sweepUnit.counter, &kZero, &maxCounter);

      uint8_t maxPeriod = 0x07;
      ImGui::SliderScalar("Period", ImGuiDataType_U8, &sweepUnit.period, &kZero, &maxPeriod);

      uint8_t maxShift = 0x07;
      ImGui::SliderScalar("Shift", ImGuiDataType_U8, &sweepUnit.shift, &kZero, &maxShift);

      ImGui::TreePop();
   }
}

void UI::renderWaveUnit(WaveUnit& waveUnit) const
{
   if (ImGui::TreeNode("Wave unit"))
   {
      ImGui::Checkbox("DAC powered", &waveUnit.dacPowered);

      uint8_t maxPosition = 31;
      ImGui::SliderScalar("Position", ImGuiDataType_U8, &waveUnit.position, &kZero, &maxPosition);

      uint8_t maxVolumeCode = 0x03;
      ImGui::SliderScalar("Volume code", ImGuiDataType_U8, &waveUnit.volumeCode, &kZero, &maxVolumeCode);

      static const auto kHistogramGetter = [](void* data, int position)
      {
         WaveUnit* waveUnit = reinterpret_cast<WaveUnit*>(data);

         uint8_t sampleIndex = position / 2;
         uint8_t value = waveUnit->waveTable[sampleIndex];
         uint8_t sample = position % 2 == 0 ? ((value & 0xF0) >> 4) : (value & 0x0F);

         return static_cast<float>(sample);
      };

      ImGui::PlotHistogram("Wave table", kHistogramGetter, &waveUnit, static_cast<int>(waveUnit.waveTable.size() * 2), 0, nullptr, 0.0f, 15.0f, ImVec2(0, 100));

      ImGui::TreePop();
   }
}

void UI::renderLFSRUnit(LFSRUnit& lfsrUnit) const
{
   if (ImGui::TreeNode("LFSR unit"))
   {
      ImGui::Checkbox("Width mode", &lfsrUnit.widthMode);

      uint8_t maxClockShift = 0x0F;
      ImGui::SliderScalar("Clock shift", ImGuiDataType_U8, &lfsrUnit.clockShift, &kZero, &maxClockShift);

      uint8_t maxDivisorCode = 0x07;
      ImGui::SliderScalar("Divisor code", ImGuiDataType_U8, &lfsrUnit.divisorCode, &kZero, &maxDivisorCode);

      static const auto kHistogramGetter = [](void* data, int position)
      {
         LFSRUnit* lfsrUnit = reinterpret_cast<LFSRUnit*>(data);

         bool high = (lfsrUnit->lfsr & (0x01 << position)) == 0;
         return high ? 1.0f : 0.0f;
      };

      ImGui::PlotHistogram("Linear feedback shift register", kHistogramGetter, &lfsrUnit, 16, 0, nullptr, 0.0f, 1.0f);

      ImGui::TreePop();
   }
}

} // namespace DotMatrix
