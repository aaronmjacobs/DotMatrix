#include "GBC/GameBoy.h"
#include "GBC/Memory.h"

#include "UI/UI.h"

#include <imgui.h>

namespace
{

uint32_t getClockSpeed(uint8_t tac)
{
   uint16_t mask = GBC::TAC::kCounterMasks[tac & GBC::TAC::InputClockSelect];
   return (GBC::CPU::kClockSpeed / mask) / 2;
}

} // namespace

void UI::renderTimerWindow(GBC::GameBoy& gameBoy) const
{
   ImGui::SetNextWindowPos(ImVec2(826.0f, 366.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(450.0f, 110.0f));
   ImGui::Begin("Timer", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Columns(2, "joypad");
   ImGui::Separator();

   unsigned int tac = gameBoy.memory.tac;
   ImGui::CheckboxFlags("Enabled", &tac, GBC::TAC::TimerStartStop);
   gameBoy.memory.tac = static_cast<uint8_t>(tac);
   uint32_t clockSpeed = getClockSpeed(gameBoy.memory.tac);
   ImGui::InputScalar("Frequency", ImGuiDataType_U32, &clockSpeed);
   ImGui::InputScalar("TIMA", ImGuiDataType_U8, &gameBoy.memory.tima);
   ImGui::NextColumn();

   ImGui::InputScalar("Counter", ImGuiDataType_U16, &gameBoy.counter);
   ImGui::InputScalar("DIV", ImGuiDataType_U8, &gameBoy.memory.div);
   ImGui::InputScalar("TMA", ImGuiDataType_U8, &gameBoy.memory.tma);
   ImGui::NextColumn();

   ImGui::Columns(1);
   ImGui::Separator();

   ImGui::End();
}
