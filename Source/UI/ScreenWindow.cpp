#include "Platform/Video/Renderer.h"

#include "UI/UI.h"

#include "UI/BeforeCoreIncludes.inl"
#  include <DotMatrixCore/GameBoy/LCDController.h>
#include "UI/AfterCoreIncludes.inl"

#include <imgui.h>

namespace DotMatrix
{

void UI::renderScreenWindow(const Renderer& renderer) const
{
   ImGui::SetNextWindowPos(ImVec2(570.0f, 5.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Screen", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Image(renderer.getTextureId(), ImVec2(kScreenWidth * 2.0f, kScreenHeight * 2.0f));

   ImGui::End();
}

} // namespace DotMatrix
