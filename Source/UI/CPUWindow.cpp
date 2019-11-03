#include "GBC/CPU.h"

#include "UI/UI.h"

#include <imgui.h>

namespace
{
   bool displayRegister(void* data, bool twoByte, const char* name, const char* id)
   {
      ImGui::AlignTextToFramePadding();
      ImGui::Text(name);

      ImGui::SameLine(0.0f, 2.0f);
      ImGui::SetNextItemWidth(22.0f * (twoByte ? 2 : 1));
      return ImGui::InputScalar(id, twoByte ? ImGuiDataType_U16 : ImGuiDataType_U8, data, nullptr, nullptr, twoByte ? "%04X" : "%02X", ImGuiInputTextFlags_CharsHexadecimal);
   }
}

void UI::renderCPUWindow(GBC::CPU& cpu) const
{
   ImGui::SetNextWindowPos(ImVec2(577.0f, 5.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(350.0f, 200.0f));
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
   uint8_t regF = cpu.reg.f;
   bool wroteF = false;
   displayRegister(&cpu.reg.a, false, "A:", "##register-a"); ImGui::SameLine(0.0f, registerPadding); wroteF = displayRegister(&regF, false, "F:", "##register-f");
   displayRegister(&cpu.reg.b, false, "B:", "##register-b"); ImGui::SameLine(0.0f, registerPadding); displayRegister(&cpu.reg.c, false, "C:", "##register-c");
   displayRegister(&cpu.reg.d, false, "D:", "##register-d"); ImGui::SameLine(0.0f, registerPadding); displayRegister(&cpu.reg.e, false, "E:", "##register-e");
   displayRegister(&cpu.reg.h, false, "H:", "##register-h"); ImGui::SameLine(0.0f, registerPadding); displayRegister(&cpu.reg.l, false, "L:", "##register-l");
   displayRegister(&cpu.reg.sp, true, "SP:", "##register-sp");
   displayRegister(&cpu.reg.pc, true, "PC:", "##register-pc");
   if (wroteF)
   {
      cpu.reg.f = regF & 0xF0;
   }
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
