#include "UI.h"

#include "GBC/GameBoy.h"
#include "GBC/LCDController.h"

#include "Platform/Video/Renderer.h"

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

void UI::render(GBC::GameBoy& gameBoy, const Renderer& renderer)
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   renderScreen(renderer);
   renderJoypad(gameBoy);
   renderCPU(gameBoy);

   ImGui::Render();

   glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
   glClear(GL_COLOR_BUFFER_BIT);

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::renderScreen(const Renderer& renderer) const
{
   ImGuiIO& io = ImGui::GetIO();
   ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
   ImGui::Begin("Screen", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Image(renderer.getTextureId(), ImVec2(GBC::kScreenWidth * 2.0f, GBC::kScreenHeight * 2.0f));

   ImGui::End();
}

void UI::renderJoypad(GBC::GameBoy& gameBoy) const
{
   ImGui::SetNextWindowPos(ImVec2(378.0f, 252.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Joypad", nullptr, ImGuiWindowFlags_NoResize);

   GBC::Joypad& joypad = gameBoy.joypad;

   ImGui::Checkbox("A", &joypad.a);
   ImGui::Checkbox("B", &joypad.b);
   ImGui::Checkbox("Start", &joypad.start);
   ImGui::Checkbox("Select", &joypad.select);
   ImGui::Checkbox("Up", &joypad.up);
   ImGui::Checkbox("Down", &joypad.down);
   ImGui::Checkbox("Left", &joypad.left);
   ImGui::Checkbox("Right", &joypad.right);

   ImGui::End();
}

void UI::renderCPU(GBC::GameBoy& gameBoy) const
{
   ImGui::SetNextWindowSize(ImVec2(350.0f, 165.0f));
   ImGui::SetNextWindowPos(ImVec2(465.0f, 20.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("CPU", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Columns(3, "cpu");
   ImGui::Separator();

   ImGui::Text("Registers");
   ImGui::NextColumn();

   ImGui::Text("Flags");
   ImGui::NextColumn();

   ImGui::Text("State");
   ImGui::NextColumn();

   ImGui::Separator();

   float registerPadding = 15.0f;
   ImGui::Text("A: %02X", gameBoy.cpu.reg.a); ImGui::SameLine(0.0f, registerPadding); ImGui::Text("F: %02X", gameBoy.cpu.reg.f);
   ImGui::Text("B: %02X", gameBoy.cpu.reg.b); ImGui::SameLine(0.0f, registerPadding); ImGui::Text("C: %02X", gameBoy.cpu.reg.c);
   ImGui::Text("D: %02X", gameBoy.cpu.reg.d); ImGui::SameLine(0.0f, registerPadding); ImGui::Text("E: %02X", gameBoy.cpu.reg.e);
   ImGui::Text("H: %02X", gameBoy.cpu.reg.h); ImGui::SameLine(0.0f, registerPadding); ImGui::Text("L: %02X", gameBoy.cpu.reg.l);
   ImGui::Text("SP: %04X", gameBoy.cpu.reg.sp);
   ImGui::Text("PC: %04X", gameBoy.cpu.reg.pc);
   ImGui::NextColumn();

   unsigned int* flags = reinterpret_cast<unsigned int*>(&gameBoy.cpu.reg.f);
   ImGui::CheckboxFlags("Zero", flags, GBC::CPU::kZero);
   ImGui::CheckboxFlags("Subtract", flags, GBC::CPU::kSub);
   ImGui::CheckboxFlags("Half Carry", flags, GBC::CPU::kHalfCarry);
   ImGui::CheckboxFlags("Carry", flags, GBC::CPU::kCarry);
   ImGui::NextColumn();

   ImGui::Checkbox("Halted", &gameBoy.cpu.halted);
   ImGui::Checkbox("Stopped", &gameBoy.cpu.stopped);
   ImGui::NextColumn();

   ImGui::Columns(1);
   ImGui::Separator();

   ImGui::End();
}
