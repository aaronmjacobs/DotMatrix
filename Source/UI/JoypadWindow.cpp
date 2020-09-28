#include "UI/UI.h"

#include "UI/BeforeCoreIncludes.inl"
#  include "GameBoy/GameBoy.h"
#include "UI/AfterCoreIncludes.inl"

#include <imgui.h>

namespace DotMatrix
{

void UI::renderJoypadWindow(Joypad& joypad) const
{
   ImGui::SetNextWindowPos(ImVec2(921.0f, 277.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(80.0f, 215.0f));
   ImGui::Begin("Joypad", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Checkbox("Up", &joypad.up);
   ImGui::Checkbox("Down", &joypad.down);
   ImGui::Checkbox("Left", &joypad.left);
   ImGui::Checkbox("Right", &joypad.right);

   ImGui::Checkbox("A", &joypad.a);
   ImGui::Checkbox("B", &joypad.b);
   ImGui::Checkbox("Start", &joypad.start);
   ImGui::Checkbox("Select", &joypad.select);

   ImGui::End();
}

} // namespace DotMatrix
