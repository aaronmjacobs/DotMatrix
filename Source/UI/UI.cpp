#include "UI/UI.h"

#include "UI/BeforeCoreIncludes.inl"
#  include "Emulator/Emulator.h"
#  include "GameBoy/GameBoy.h"
#include "UI/AfterCoreIncludes.inl"

#include <glad/glad.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

namespace DotMatrix
{

// static
int UI::getDesiredWindowWidth()
{
   return 1440;
}

// static
int UI::getDesiredWindowHeight()
{
   return 778;
}

UI::UI(GLFWwindow* window)
{
   IMGUI_CHECKVERSION();

   ImGui::CreateContext();
   ImGui::StyleColorsDark();

   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 150");
}

UI::~UI()
{
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();

   ImGui::DestroyContext();
}

void UI::render(Emulator& emulator)
{
   DM_ASSERT(emulator.gameBoy);

   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   renderScreenWindow(*emulator.renderer);
   renderEmulatorWindow(emulator);
   renderTimerWindow(*emulator.gameBoy);
   renderJoypadWindow(emulator.gameBoy->joypad);
   renderCPUWindow(emulator.gameBoy->cpu);
   renderMemoryWindow(*emulator.gameBoy);
   renderLCDControllerWindow(emulator.gameBoy->lcdController);
   renderSoundControllerWindow(emulator.gameBoy->soundController);
   renderCartridgeWindow(emulator.gameBoy->cart.get());
#if DM_WITH_DEBUGGER
   renderDebuggerWindow(*emulator.gameBoy);
#endif // DM_WITH_DEBUGGER

   ImGui::Render();

   glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
   glClear(GL_COLOR_BUFFER_BIT);

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::onRomLoaded(GameBoy& gameBoy, const char* romPath)
{
#if DM_WITH_DEBUGGER
   onRomLoaded_Debugger(gameBoy, romPath);
#endif // DM_WITH_DEBUGGER
}

// static
const uint64_t UI::kZero = 0;

} // namespace DotMatrix
