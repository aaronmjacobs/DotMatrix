#include "Emulator.h"

#include "GBC/GameBoy.h"

#include "UI/UI.h"

#include <glad/glad.h>

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

// static
int UI::getDesiredWindowWidth()
{
   return 1280;
}

// static
int UI::getDesiredWindowHeight()
{
   return 720;
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
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   renderScreenWindow(*emulator.renderer);
   renderEmulatorWindow(emulator);
   renderTimerWindow(*emulator.gameBoy);
   renderJoypadWindow(emulator.gameBoy->joypad);
   renderCPUWindow(emulator.gameBoy->cpu);
   renderMemoryWindow(emulator.gameBoy->memory);
   renderSoundControllerWindow(emulator.gameBoy->soundController);
   renderCartridgeWindow(emulator.gameBoy->cart.get());
   renderDebuggerWindow(*emulator.gameBoy);

   ImGui::Render();

   glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
   glClear(GL_COLOR_BUFFER_BIT);

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// static
const uint64_t UI::kZero = 0;
