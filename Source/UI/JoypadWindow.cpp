#include "GBC/GameBoy.h"

#include "UI/UI.h"

#include <imgui.h>

void UI::renderJoypadWindow(GBC::Joypad& joypad) const
{
   ImGui::SetNextWindowPos(ImVec2(646.0f, 353.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(175.0f, 135.0f));
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
