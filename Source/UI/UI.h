#pragma once

#if !GBC_WITH_UI
#   error "Including UI config header, but GBC_WITH_UI is not set!"
#endif // !GBC_WITH_UI

#include <cstdint>
#include <vector>

class Emulator;
class Renderer;

namespace GBC
{
class DutyUnit;
class EnvelopeUnit;
class GameBoy;
class LengthUnit;
class LFSRUnit;
class SoundChannel;
class SweepUnit;
class WaveUnit;
template<typename T>
class SoundTimer;
} // namespace GBC

struct GLFWwindow;

class UI
{
public:
   static int getDesiredWindowWidth();
   static int getDesiredWindowHeight();

   UI(GLFWwindow* window);
   ~UI();

   void render(Emulator& emulator);

private:
   void renderScreen(const Renderer& renderer) const;
   void renderEmulator(Emulator& emulator) const;
   void renderJoypad(GBC::GameBoy& gameBoy) const;
   void renderCPU(GBC::GameBoy& gameBoy) const;
   void renderMemory(GBC::GameBoy& gameBoy) const;
   void renderSoundController(GBC::GameBoy& gameBoy) const;

   void renderSoundChannel(GBC::SoundChannel& soundChannel, std::vector<float>& samples, int offset) const;
   template<typename T>
   void renderSoundTimer(GBC::SoundTimer<T>& soundTimer, uint32_t maxPeriod, bool displayNote) const;
   void renderDutyUnit(GBC::DutyUnit& dutyUnit) const;
   void renderLengthUnit(GBC::LengthUnit& lengthUnit) const;
   void renderEnvelopeUnit(GBC::EnvelopeUnit& envelopeUnit) const;
   void renderSweepUnit(GBC::SweepUnit& sweepUnit) const;
   void renderWaveUnit(GBC::WaveUnit& waveUnit) const;
   void renderLFSRUnit(GBC::LFSRUnit& lfsrUnit) const;
};
