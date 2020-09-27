#pragma once

#if !DM_WITH_UI
#  error "Including UI header, but DM_WITH_UI is not set!"
#endif // !DM_WITH_UI

#include <cstdint>
#include <vector>

class Renderer;
struct GLFWwindow;

namespace DotMatrix
{

class Cartridge;
class CPU;
class DutyUnit;
class Emulator;
class EnvelopeUnit;
class GameBoy;
class LCDController;
class LengthUnit;
class LFSRUnit;
class SoundChannel;
class SoundController;
class SweepUnit;
class WaveUnit;
template<typename T>
class SoundTimer;
struct Joypad;

class UI
{
public:
   static int getDesiredWindowWidth();
   static int getDesiredWindowHeight();

   UI(GLFWwindow* window);
   ~UI();

   void render(Emulator& emulator);

   void onRomLoaded(GameBoy& gameBoy, const char* romPath);

private:
   static const uint64_t kZero;

   void renderScreenWindow(const Renderer& renderer) const;
   void renderEmulatorWindow(Emulator& emulator) const;
   void renderTimerWindow(GameBoy& gameBoy) const;
   void renderJoypadWindow(Joypad& joypad) const;
   void renderCPUWindow(CPU& cpu) const;
   void renderMemoryWindow(GameBoy& gameBoy) const;
   void renderLCDControllerWindow(LCDController& lcdController) const;
   void renderSoundControllerWindow(SoundController& soundController) const;
   void renderCartridgeWindow(Cartridge* cart) const;
#if DM_WITH_DEBUGGER
   void renderDebuggerWindow(GameBoy& gameBoy) const;
#endif // DM_WITH_DEBUGGER

   void renderSoundChannel(SoundChannel& soundChannel, std::vector<float>& samples, int offset) const;
   template<typename T>
   void renderSoundTimer(SoundTimer<T>& soundTimer, uint32_t maxPeriod, bool displayNote) const;
   void renderDutyUnit(DutyUnit& dutyUnit) const;
   void renderLengthUnit(LengthUnit& lengthUnit) const;
   void renderEnvelopeUnit(EnvelopeUnit& envelopeUnit) const;
   void renderSweepUnit(SweepUnit& sweepUnit) const;
   void renderWaveUnit(WaveUnit& waveUnit) const;
   void renderLFSRUnit(LFSRUnit& lfsrUnit) const;

#if DM_WITH_DEBUGGER
   void onRomLoaded_Debugger(GameBoy& gameBoy, const char* romPath) const;
#endif // DM_WITH_DEBUGGER
};

} // namespace DotMatrix
