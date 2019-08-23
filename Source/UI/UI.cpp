#include "UI.h"

#include "GBC/GameBoy.h"
#include "GBC/LCDController.h"

#include "Platform/Video/Renderer.h"

#include <glad/glad.h>

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include <cstdint>

namespace
{

struct MemoryHelper
{
   GBC::Memory* memory = nullptr;
   uint16_t address = 0;
};

MemoryEditor::u8 readMemory(const MemoryEditor::u8* data, size_t offset)
{
   const MemoryHelper* memoryHelper = reinterpret_cast<const MemoryHelper*>(data);

   return memoryHelper->memory->readDirect(memoryHelper->address + static_cast<uint16_t>(offset));
}

void writeMemory(MemoryEditor::u8* data, size_t offset, MemoryEditor::u8 value)
{
   MemoryHelper* memoryHelper = reinterpret_cast<MemoryHelper*>(data);

   memoryHelper->memory->writeDirect(memoryHelper->address + static_cast<uint16_t>(offset), value);
}

MemoryEditor createMemoryEditor()
{
   MemoryEditor memoryEditor;

   memoryEditor.ReadFn = readMemory;
   memoryEditor.WriteFn = writeMemory;

   return memoryEditor;
}

void renderMemoryRegion(GBC::Memory& memory, const char* name, const char* description, uint16_t size, uint16_t& address)
{
   if (ImGui::BeginTabItem(name))
   {
      ImGui::Text("%s", description);
      ImGui::Separator();

      MemoryHelper memoryHelper;
      memoryHelper.memory = &memory;
      memoryHelper.address = address;

      static MemoryEditor memoryEditor = createMemoryEditor();
      memoryEditor.DrawContents(&memoryHelper, size, address);

      ImGui::EndTabItem();
   }

   address += size;
}

float audioSampleGetter(void* data, int index)
{
   std::vector<float>* floatSamples = reinterpret_cast<std::vector<float>*>(data);

   return (*floatSamples)[index];
}

} // namespace

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
   renderMemory(gameBoy);
   renderSoundController(gameBoy);

   ImGui::Render();

   glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
   glClear(GL_COLOR_BUFFER_BIT);

   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UI::renderScreen(const Renderer& renderer) const
{
   ImGuiIO& io = ImGui::GetIO();
   ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - 100.0f, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
   ImGui::Begin("Screen", nullptr, ImGuiWindowFlags_NoResize);

   ImGui::Image(renderer.getTextureId(), ImVec2(GBC::kScreenWidth * 2.0f, GBC::kScreenHeight * 2.0f));

   ImGui::End();
}

void UI::renderJoypad(GBC::GameBoy& gameBoy) const
{
   ImGui::SetNextWindowPos(ImVec2(278.0f, 252.0f), ImGuiCond_FirstUseEver);
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
   ImGui::SetNextWindowPos(ImVec2(365.0f, 20.0f), ImGuiCond_FirstUseEver);
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

void UI::renderMemory(GBC::GameBoy& gameBoy) const
{
   ImGui::SetNextWindowSize(ImVec2(560.0f, 524.0f));
   ImGui::SetNextWindowPos(ImVec2(718.0f, 11.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Memory");

   if (ImGui::BeginTabBar("MemoryTabBar"))
   {
      GBC::Memory& memory = gameBoy.memory;
      uint16_t address = 0x0000;

      renderMemoryRegion(memory, "romb", "Permanently-mapped ROM bank", 0x4000, address);
      renderMemoryRegion(memory, "roms", "Switchable ROM bank", 0x4000, address);
      renderMemoryRegion(memory, "vram", "Video RAM", 0x2000, address);
      renderMemoryRegion(memory, "eram", "Switchable external RAM bank", 0x2000, address);
      renderMemoryRegion(memory, "ram0", "Working RAM bank 0", 0x1000, address);
      renderMemoryRegion(memory, "ram1", "Working RAM bank 1", 0x1000, address);
      renderMemoryRegion(memory, "ramm", "Mirror of working ram", 0x1E00, address);
      renderMemoryRegion(memory, "oam", "Sprite attribute table", 0x0100, address);
      renderMemoryRegion(memory, "io", "I/O device mappings", 0x0080, address);
      renderMemoryRegion(memory, "ramh", "High RAM area and interrupt enable register", 0x0080, address);

      ImGui::EndTabBar();
   }

   ImGui::End();
}

void UI::renderSoundController(GBC::GameBoy& gameBoy) const
{
   ImGui::Begin("Sound Controller");

   static const std::size_t kNumSamples = GBC::SoundController::kSampleRate / 4;
   static std::size_t offset = 0;

   if (ImGui::CollapsingHeader("Output"))
   {
      static std::vector<float> leftSamples(kNumSamples);
      static std::vector<float> rightSamples(kNumSamples);

      const std::vector<GBC::AudioSample>& audioData = gameBoy.getSoundController().buffers[!gameBoy.getSoundController().activeBufferIndex];

      for (const GBC::AudioSample& audioSample : audioData)
      {
         leftSamples[offset] = static_cast<float>(audioSample.left) / -SHRT_MIN;
         rightSamples[offset] = static_cast<float>(audioSample.right) / -SHRT_MIN;
         offset = (offset + 1) % kNumSamples;
      }

      ImGui::PlotLines("Left", audioSampleGetter, &leftSamples, kNumSamples, offset, nullptr, -1.0f, 1.0f, ImVec2(500.0f, 100.0f));
      ImGui::PlotLines("Right", audioSampleGetter, &rightSamples, kNumSamples, offset, nullptr, -1.0f, 1.0f, ImVec2(500.0f, 100.0f));
   }

   ImGui::End();
}
