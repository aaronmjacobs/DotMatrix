#include "UI/UI.h"

#include "UI/BeforeCoreIncludes.inl"
#  include <DotMatrixCore/GameBoy/GameBoy.h>
#include "UI/AfterCoreIncludes.inl"

#include <imgui.h>

namespace DotMatrix
{

void UI::renderJoypadWindow(Joypad& joypad) const
{
   ImGui::SetNextWindowPos(ImVec2(539.0f, 553.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(171.0f, 135.0f));
   ImGui::Begin("Joypad", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Columns(2, "joypad");
   ImGui::Separator();

   ImGui::Checkbox("Up", &joypad.up);
   ImGui::Checkbox("Down", &joypad.down);
   ImGui::Checkbox("Left", &joypad.left);
   ImGui::Checkbox("Right", &joypad.right);
   ImGui::NextColumn();

   ImGui::Checkbox("A", &joypad.a);
   ImGui::Checkbox("B", &joypad.b);
   ImGui::Checkbox("Start", &joypad.start);
   ImGui::Checkbox("Select", &joypad.select);
   ImGui::NextColumn();

   ImGui::Columns(1);
   ImGui::Separator();

   ImGui::End();
}

} // namespace DotMatrix
