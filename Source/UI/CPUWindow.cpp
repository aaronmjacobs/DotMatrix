#include "GBC/CPU.h"

#include "UI/UI.h"

#include <imgui.h>

void UI::renderCPUWindow(GBC::CPU& cpu) const
{
   ImGui::SetNextWindowPos(ImVec2(577.0f, 5.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(350.0f, 165.0f));
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
   ImGui::Text("A: %02X", cpu.reg.a); ImGui::SameLine(0.0f, registerPadding); ImGui::Text("F: %02X", cpu.reg.f);
   ImGui::Text("B: %02X", cpu.reg.b); ImGui::SameLine(0.0f, registerPadding); ImGui::Text("C: %02X", cpu.reg.c);
   ImGui::Text("D: %02X", cpu.reg.d); ImGui::SameLine(0.0f, registerPadding); ImGui::Text("E: %02X", cpu.reg.e);
   ImGui::Text("H: %02X", cpu.reg.h); ImGui::SameLine(0.0f, registerPadding); ImGui::Text("L: %02X", cpu.reg.l);
   ImGui::Text("SP: %04X", cpu.reg.sp);
   ImGui::Text("PC: %04X", cpu.reg.pc);
   ImGui::NextColumn();

   unsigned int* flags = reinterpret_cast<unsigned int*>(&cpu.reg.f);
   ImGui::CheckboxFlags("Zero", flags, Enum::cast(GBC::CPU::Flag::Zero));
   ImGui::CheckboxFlags("Subtract", flags, Enum::cast(GBC::CPU::Flag::Sub));
   ImGui::CheckboxFlags("Half Carry", flags, Enum::cast(GBC::CPU::Flag::HalfCarry));
   ImGui::CheckboxFlags("Carry", flags, Enum::cast(GBC::CPU::Flag::Carry));
   ImGui::NextColumn();

   ImGui::Checkbox("Halted", &cpu.halted);
   ImGui::Checkbox("Stopped", &cpu.stopped);
   ImGui::NextColumn();

   ImGui::Columns(1);
   ImGui::Separator();

   ImGui::End();
}
