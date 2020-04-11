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
class Cartridge;
class CPU;
class DutyUnit;
class EnvelopeUnit;
class GameBoy;
class LengthUnit;
class LFSRUnit;
class SoundChannel;
class SoundController;
class SweepUnit;
class WaveUnit;
template<typename T>
class SoundTimer;
struct Joypad;
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

   void onRomLoaded(const char* romPath);

private:
   static const uint64_t kZero;

   bool isNewGameBoy() const
   {
      return newGameBoy;
   }

   void renderScreenWindow(const Renderer& renderer) const;
   void renderEmulatorWindow(Emulator& emulator) const;
   void renderTimerWindow(GBC::GameBoy& gameBoy) const;
   void renderJoypadWindow(GBC::Joypad& joypad) const;
   void renderCPUWindow(GBC::CPU& cpu) const;
   void renderMemoryWindow(GBC::GameBoy& gameBoy) const;
   void renderSoundControllerWindow(GBC::SoundController& soundController) const;
   void renderCartridgeWindow(GBC::Cartridge* cart) const;
#if GBC_WITH_DEBUGGER
   void renderDebuggerWindow(GBC::GameBoy& gameBoy) const;
#endif // GBC_WITH_DEBUGGER

   void renderSoundChannel(GBC::SoundChannel& soundChannel, std::vector<float>& samples, int offset) const;
   template<typename T>
   void renderSoundTimer(GBC::SoundTimer<T>& soundTimer, uint32_t maxPeriod, bool displayNote) const;
   void renderDutyUnit(GBC::DutyUnit& dutyUnit) const;
   void renderLengthUnit(GBC::LengthUnit& lengthUnit) const;
   void renderEnvelopeUnit(GBC::EnvelopeUnit& envelopeUnit) const;
   void renderSweepUnit(GBC::SweepUnit& sweepUnit) const;
   void renderWaveUnit(GBC::WaveUnit& waveUnit) const;
   void renderLFSRUnit(GBC::LFSRUnit& lfsrUnit) const;

#if GBC_WITH_DEBUGGER
   void onRomLoaded_Debugger(const char* romPath) const;
#endif // GBC_WITH_DEBUGGER

   GBC::GameBoy* lastGameBoy;
   bool newGameBoy;
};
