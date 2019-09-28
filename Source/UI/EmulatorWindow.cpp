#include "Emulator.h"

#include "Core/Math.h"

#include "UI/UI.h"

#include <imgui.h>

void UI::renderEmulatorWindow(Emulator& emulator) const
{
   ImGui::Begin("Emulator");

   float timeScale = static_cast<float>(emulator.timeScale);
   ImGui::SliderFloat("Time scale", &timeScale, 0.0f, 1.0f, nullptr, 2.0f);
   emulator.timeScale = timeScale;

   uint64_t clockSpeed = Math::round<uint64_t>(GBC::CPU::kClockSpeed * timeScale);
   ImGui::Text("Clock speed: %llu", clockSpeed);

   if (ImGui::Button("Step"))
   {
      emulator.timeScale = 0.0;
      emulator.gameBoy->cpu.tick();
   }

   ImGui::End();
}
