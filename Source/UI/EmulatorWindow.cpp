#include "UI/UI.h"

#include "UI/BeforeCoreIncludes.inl"
#  include "Core/Math.h"
#  include "Emulator/Emulator.h"
#include "UI/AfterCoreIncludes.inl"

#include <imgui.h>

namespace DotMatrix
{

void UI::renderEmulatorWindow(Emulator& emulator) const
{
   ImGui::SetNextWindowPos(ImVec2(580.0f, 559.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(290.0f, 90.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Emulator");

   float timeScale = static_cast<float>(emulator.timeScale);
   ImGui::SliderFloat("Time scale", &timeScale, 0.0f, 1.0f, nullptr, ImGuiSliderFlags_Logarithmic);
   emulator.timeScale = timeScale;

   uint64_t clockSpeed = Math::round<uint64_t>(CPU::kClockSpeed * timeScale);
   ImGui::Text("Clock speed:  %llu", clockSpeed);

   uint64_t totalCycles = emulator.gameBoy ? emulator.gameBoy->totalCycles : 0;
   ImGui::Text("Total cycles: %llu", totalCycles);

   ImGui::End();
}

} // namespace DotMatrix
