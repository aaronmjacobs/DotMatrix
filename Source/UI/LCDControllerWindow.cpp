#include "UI/UI.h"

#include "UI/BeforeCoreIncludes.inl"
#  include "Emulator/Emulator.h"
#  include "GameBoy/LCDController.h"
#include "UI/AfterCoreIncludes.inl"

#include <imgui.h>

namespace DotMatrix
{

namespace
{
   struct Color
   {
      float r = 0.0f;
      float g = 0.0f;
      float b = 0.0f;
   };

   Color pixelToColor(Pixel pixel)
   {
      Color color;

      color.r = pixel.r / 255.0f;
      color.g = pixel.g / 255.0f;
      color.b = pixel.b / 255.0f;

      return color;
   }

   std::array<Color, 4> decodeColors(const LCDController& lcdController, uint8_t palette)
   {
      std::array<uint8_t, 4> paletteColors = lcdController.extractPaletteColors(palette);

      DM_ASSERT(paletteColors[0] < 4 && paletteColors[1] < 4 && paletteColors[2] < 4 && paletteColors[3] < 4);
      std::array<Pixel, 4> framebufferColors =
      {
         kFramebufferColors[paletteColors[0]],
         kFramebufferColors[paletteColors[1]],
         kFramebufferColors[paletteColors[2]],
         kFramebufferColors[paletteColors[3]],
      };

      std::array<Color, 4> colors =
      {
         pixelToColor(framebufferColors[0]),
         pixelToColor(framebufferColors[1]),
         pixelToColor(framebufferColors[2]),
         pixelToColor(framebufferColors[3])
      };
      return colors;
   }
}

void UI::renderLCDControllerWindow(LCDController& lcdController) const
{
   ImGui::SetNextWindowPos(ImVec2(935.0f, 5.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(500.0f, 180.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("LCD Controller");

   if (ImGui::BeginTabBar("LCDControllerTabBar"))
   {
      if (ImGui::BeginTabItem("Control Register"))
      {
         ImGui::Columns(2, "control register");
         ImGui::Separator();

         ImGui::Checkbox("LCD display enabled", &lcdController.controlRegister.lcdDisplayEnabled);
         ImGui::Checkbox("Window use upper tile map", &lcdController.controlRegister.windowUseUpperTileMap);
         ImGui::Checkbox("Window display enabled", &lcdController.controlRegister.windowDisplayEnabled);
         ImGui::Checkbox("BG / window unsigned tile data", &lcdController.controlRegister.bgAndWindowUseUnsignedTileData);

         ImGui::NextColumn();

         ImGui::Checkbox("BG use upper tile map", &lcdController.controlRegister.bgUseUpperTileMap);
         ImGui::Checkbox("Use large sprite side", &lcdController.controlRegister.useLargeSpriteSize);
         ImGui::Checkbox("Sprite display enabled", &lcdController.controlRegister.spriteDisplayEnabled);
         ImGui::Checkbox("BG / window display enabled", &lcdController.controlRegister.bgWindowDisplayEnabled);

         ImGui::Columns(1);
         ImGui::Separator();

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Status Register"))
      {
         ImGui::Columns(2, "status register");
         ImGui::Separator();

         ImGui::Checkbox("Coincidence interrupt", &lcdController.statusRegister.coincidenceInterrupt);
         ImGui::Checkbox("OAM interrupt", &lcdController.statusRegister.oamInterrupt);
         ImGui::Checkbox("VBlank interrupt", &lcdController.statusRegister.vBlankInterrupt);

         ImGui::NextColumn();

         ImGui::Checkbox("HBlank interrupt", &lcdController.statusRegister.hBlankInterrupt);
         ImGui::Checkbox("Coincidence flag", &lcdController.statusRegister.coincidenceFlag);

         static const std::array<const char*, 4> kModeNames =
         {
            "0: HBlank",
            "1: VBlank",
            "2: Search OAM",
            "3: Data transfer"
         };
         int mode = static_cast<int>(lcdController.statusRegister.mode);
         ImGui::Combo("Mode", &mode, kModeNames.data(), static_cast<int>(kModeNames.size()));
         DM_ASSERT(mode < 4);
         lcdController.statusRegister.mode = static_cast<LCDController::Mode>(mode);

         ImGui::Columns(1);
         ImGui::Separator();

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Other Registers"))
      {
         static const uint8_t kMaxScroll = 255;
         static const uint8_t kMaxLy = 153;

         ImGui::Columns(2, "other registers");
         ImGui::Separator();

         ImGui::SliderScalar("SCY", ImGuiDataType_U8, &lcdController.scy, &kZero, &kMaxScroll);
         ImGui::SliderScalar("SCX", ImGuiDataType_U8, &lcdController.scx, &kZero, &kMaxScroll);
         ImGui::SliderScalar("LY", ImGuiDataType_U8, &lcdController.ly, &kZero, &kMaxLy);
         ImGui::SliderScalar("LYC", ImGuiDataType_U8, &lcdController.lyc, &kZero, &kMaxLy);
         ImGui::InputScalar("DMA", ImGuiDataType_U8, &lcdController.dma);

         ImGui::NextColumn();

         ImGuiColorEditFlags colorEditFlags = ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop;

#define SHOW_PALETTE_VIEWER(palette_name, palette_variable)\
         ImGui::SetNextItemWidth(50.0f);\
         ImGui::InputScalar(palette_name, ImGuiDataType_U8, &lcdController.palette_variable);\
         std::array<Color, 4> palette_variable##Colors = decodeColors(lcdController, lcdController.palette_variable);\
         ImGui::SameLine();\
         ImGui::ColorEdit3(palette_name"[0]", &palette_variable##Colors[0].r, colorEditFlags);\
         ImGui::SameLine();\
         ImGui::ColorEdit3(palette_name"[1]", &palette_variable##Colors[1].r, colorEditFlags);\
         ImGui::SameLine();\
         ImGui::ColorEdit3(palette_name"[2]", &palette_variable##Colors[2].r, colorEditFlags);\
         ImGui::SameLine();\
         ImGui::ColorEdit3(palette_name"[3]", &palette_variable##Colors[3].r, colorEditFlags)

         SHOW_PALETTE_VIEWER("BGP ", bgp);
         SHOW_PALETTE_VIEWER("OBP0", obp0);
         SHOW_PALETTE_VIEWER("OBP1", obp1);

#undef SHOW_PALETTE_VIEWER

         ImGui::SliderScalar("WY", ImGuiDataType_U8, &lcdController.wy, &kZero, &kMaxScroll);
         ImGui::SliderScalar("WX", ImGuiDataType_U8, &lcdController.wx, &kZero, &kMaxScroll);

         ImGui::Columns(1);
         ImGui::Separator();

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("DMA"))
      {
         ImGui::Columns(2, "dma");
         ImGui::Separator();

         ImGui::Checkbox("DMA requested", &lcdController.dmaRequested);
         ImGui::Checkbox("DMA pending", &lcdController.dmaPending);
         ImGui::Checkbox("DMA in progress", &lcdController.dmaInProgress);

         ImGui::NextColumn();

         ImGui::InputScalar("DMA index", ImGuiDataType_U8, &lcdController.dmaIndex);
         ImGui::InputScalar("DMA source", ImGuiDataType_U16, &lcdController.dmaSource);
         ImGui::InputScalar("DMA", ImGuiDataType_U8, &lcdController.dma);

         ImGui::Columns(1);
         ImGui::Separator();

         ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
   }

   ImGui::End();
}

} // namespace DotMatrix
