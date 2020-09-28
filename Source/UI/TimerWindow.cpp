#include "UI/UI.h"

#include "UI/BeforeCoreIncludes.inl"
#  include "GameBoy/GameBoy.h"
#include "UI/AfterCoreIncludes.inl"

#include <imgui.h>

namespace DotMatrix
{

namespace
{
   uint32_t getClockSpeed(uint8_t tac)
   {
      uint16_t mask = TAC::kCounterMasks[tac & TAC::InputClockSelect];
      return (CPU::kClockSpeed / mask) / 2;
   }
}

void UI::renderTimerWindow(GameBoy& gameBoy) const
{
   ImGui::SetNextWindowPos(ImVec2(539.0f, 412.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(381.0f, 110.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Timer");

   ImGui::Columns(2, "joypad");
   ImGui::Separator();

   ImGui::PushItemWidth(100.0f);
   unsigned int tac = gameBoy.tac;
   ImGui::CheckboxFlags("Enabled", &tac, TAC::TimerStartStop);
   gameBoy.tac = static_cast<uint8_t>(tac);
   uint32_t clockSpeed = getClockSpeed(gameBoy.tac);
   ImGui::InputScalar("Frequency", ImGuiDataType_U32, &clockSpeed);
   ImGui::InputScalar("TIMA", ImGuiDataType_U8, &gameBoy.tima);
   ImGui::NextColumn();

   ImGui::PushItemWidth(100.0f);
   ImGui::InputScalar("Counter", ImGuiDataType_U16, &gameBoy.counter);
   uint8_t div = gameBoy.readDirect(0xFF04);
   if (ImGui::InputScalar("DIV", ImGuiDataType_U8, &div))
   {
      gameBoy.writeDirect(0xFF04, div);
   }
   ImGui::InputScalar("TMA", ImGuiDataType_U8, &gameBoy.tma);
   ImGui::NextColumn();

   ImGui::Columns(1);
   ImGui::Separator();

   ImGui::End();
}

} // namespace DotMatrix
