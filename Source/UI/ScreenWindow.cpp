#include "GBC/LCDController.h"

#include "Platform/Video/Renderer.h"

#include "UI/UI.h"

#include <imgui.h>

void UI::renderScreenWindow(const Renderer& renderer) const
{
   ImGui::SetNextWindowPos(ImVec2(939.0f, 5.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Screen", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Image(renderer.getTextureId(), ImVec2(GBC::kScreenWidth * 2.0f, GBC::kScreenHeight * 2.0f));

   ImGui::End();
}
