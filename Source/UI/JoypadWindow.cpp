#include "GBC/GameBoy.h"

#include "UI/UI.h"

#include <imgui.h>

void UI::renderJoypadWindow(GBC::Joypad& joypad) const
{
   ImGui::SetNextWindowSize(ImVec2(175.0f, 135.0f));
   ImGui::SetNextWindowPos(ImVec2(454.0f, 525.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Joypad", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Columns(2, "joypad");
   ImGui::Separator();

   ImGui::Checkbox("Up", &joypad.up);
   ImGui::NextColumn();

   ImGui::Checkbox("A", &joypad.a);
   ImGui::NextColumn();

   ImGui::Checkbox("Down", &joypad.down);
   ImGui::NextColumn();

   ImGui::Checkbox("B", &joypad.b);
   ImGui::NextColumn();

   ImGui::Checkbox("Left", &joypad.left);
   ImGui::NextColumn();

   ImGui::Checkbox("Start", &joypad.start);
   ImGui::NextColumn();

   ImGui::Checkbox("Right", &joypad.right);
   ImGui::NextColumn();

   ImGui::Checkbox("Select", &joypad.select);
   ImGui::NextColumn();

   ImGui::Columns(1);
   ImGui::Separator();

   ImGui::End();
}
