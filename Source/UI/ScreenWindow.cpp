#include "GBC/LCDController.h"

#include "Platform/Video/Renderer.h"

#include "UI/UI.h"

#include <imgui.h>

void UI::renderScreenWindow(const Renderer& renderer) const
{
   ImGuiIO& io = ImGui::GetIO();
   ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 100.0f, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
   ImGui::Begin("Screen", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Image(renderer.getTextureId(), ImVec2(GBC::kScreenWidth * 2.0f, GBC::kScreenHeight * 2.0f));

   ImGui::End();
}
