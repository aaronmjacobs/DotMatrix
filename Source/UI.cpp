#include "UI.h"

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

   {
      ImGui::Begin("Screen");

      ImGui::Image(renderer.getTextureId(), ImVec2(GBC::kScreenWidth * 2.0f, GBC::kScreenHeight * 2.0f));

      ImGui::End();
   }

   ImGui::Render();

   glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
   glClear(GL_COLOR_BUFFER_BIT);

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
