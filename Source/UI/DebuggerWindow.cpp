#include "GBC/CPU.h"
#include "GBC/GameBoy.h"
#include "GBC/Operations.h"

#include "UI/UI.h"

#include <imgui.h>

#include <iomanip>
#include <sstream>
#include <string>

namespace
{
   bool usesImm8(GBC::Operation operation)
   {
      return operation.param1 == GBC::Opr::Imm8 || operation.param2 == GBC::Opr::Imm8
         || operation.param1 == GBC::Opr::DerefImm8 || operation.param2 == GBC::Opr::DerefImm8
         || operation.param1 == GBC::Opr::Imm8Signed || operation.param2 == GBC::Opr::Imm8Signed;
   }

   bool usesImm16(GBC::Operation operation)
   {
      return operation.param1 == GBC::Opr::Imm16 || operation.param2 == GBC::Opr::Imm16
         || operation.param1 == GBC::Opr::DerefImm16 || operation.param2 == GBC::Opr::DerefImm16;
   }

   const char* getInsText(GBC::Ins ins)
   {
      switch (ins)
      {
      case GBC::Ins::Invalid: return "Invalid";

      case GBC::Ins::LD: return "LD";
      case GBC::Ins::LDD: return "LDD";
      case GBC::Ins::LDI: return "LDI";
      case GBC::Ins::LDH: return "LDH";
      case GBC::Ins::LDHL: return "LDHL";
      case GBC::Ins::PUSH: return "PUSH";
      case GBC::Ins::POP: return "POP";

      case GBC::Ins::ADD: return "ADD";
      case GBC::Ins::ADC: return "ADC";
      case GBC::Ins::SUB: return "SUB";
      case GBC::Ins::SBC: return "SBC";
      case GBC::Ins::AND: return "AND";
      case GBC::Ins::OR: return "OR";
      case GBC::Ins::XOR: return "XOR";
      case GBC::Ins::CP: return "CP";
      case GBC::Ins::INC: return "INC";
      case GBC::Ins::DEC: return "DEC";

      case GBC::Ins::SWAP: return "SWAP";
      case GBC::Ins::DAA: return "DAA";
      case GBC::Ins::CPL: return "CPL";
      case GBC::Ins::CCF: return "CCF";
      case GBC::Ins::SCF: return "SCF";
      case GBC::Ins::NOP: return "NOP";
      case GBC::Ins::HALT: return "HALT";
      case GBC::Ins::STOP: return "STOP";
      case GBC::Ins::DI: return "DI";
      case GBC::Ins::EI: return "EI";

      case GBC::Ins::RLCA: return "RLCA";
      case GBC::Ins::RLA: return "RLA";
      case GBC::Ins::RRCA: return "RRCA";
      case GBC::Ins::RRA: return "RRA";
      case GBC::Ins::RLC: return "RLC";
      case GBC::Ins::RL: return "RL";
      case GBC::Ins::RRC: return "RRC";
      case GBC::Ins::RR: return "RR";
      case GBC::Ins::SLA: return "SLA";
      case GBC::Ins::SRA: return "SRA";
      case GBC::Ins::SRL: return "SRL";

      case GBC::Ins::BIT: return "BIT";
      case GBC::Ins::SET: return "SET";
      case GBC::Ins::RES: return "RES";

      case GBC::Ins::JP: return "JP";
      case GBC::Ins::JR: return "JR";

      case GBC::Ins::CALL: return "CALL";

      case GBC::Ins::RST: return "RST";

      case GBC::Ins::RET: return "RET";
      case GBC::Ins::RETI: return "RETI";

      case GBC::Ins::PREFIX: return "PREFIX";

      default: return "Unknown";
      }
   }

   const char* getOprText(GBC::Opr opr)
   {
      switch (opr)
      {
      case GBC::Opr::None: return nullptr;

      case GBC::Opr::A: return "A";
      case GBC::Opr::F: return "F";
      case GBC::Opr::B: return "B";
      case GBC::Opr::C: return "C";
      case GBC::Opr::D: return "D";
      case GBC::Opr::E: return "E";
      case GBC::Opr::H: return "H";
      case GBC::Opr::L: return "L";

      case GBC::Opr::AF: return "AF";
      case GBC::Opr::BC: return "BC";
      case GBC::Opr::DE: return "DE";
      case GBC::Opr::HL: return "HL";
      case GBC::Opr::SP: return "SP";
      case GBC::Opr::PC: return "PC";

      case GBC::Opr::Imm8: return "d8";
      case GBC::Opr::Imm16: return "d16";
      case GBC::Opr::Imm8Signed: return "r8";

      case GBC::Opr::DerefC: return "(C)";

      case GBC::Opr::DerefBC: return "(BC)";
      case GBC::Opr::DerefDE: return "(DE)";
      case GBC::Opr::DerefHL: return "(HL)";

      case GBC::Opr::DerefImm8: return "(a8)";
      case GBC::Opr::DerefImm16: return "(a16)";

      case GBC::Opr::CB: return "CB";

      case GBC::Opr::FlagC: return "C";
      case GBC::Opr::FlagNC: return "NC";
      case GBC::Opr::FlagZ: return "Z";
      case GBC::Opr::FlagNZ: return "NZ";

      case GBC::Opr::Bit0: return "0";
      case GBC::Opr::Bit1: return "1";
      case GBC::Opr::Bit2: return "2";
      case GBC::Opr::Bit3: return "3";
      case GBC::Opr::Bit4: return "4";
      case GBC::Opr::Bit5: return "5";
      case GBC::Opr::Bit6: return "6";
      case GBC::Opr::Bit7: return "7";

      case GBC::Opr::Rst00H: return "00H";
      case GBC::Opr::Rst08H: return "08H";
      case GBC::Opr::Rst10H: return "10H";
      case GBC::Opr::Rst18H: return "18H";
      case GBC::Opr::Rst20H: return "20H";
      case GBC::Opr::Rst28H: return "28H";
      case GBC::Opr::Rst30H: return "30H";
      case GBC::Opr::Rst38H: return "38H";

      default: return "Unknown";
      }
   }

   enum class OperationType
   {
      Normal,
      Current,
      CB,
      Immediate
   };

   bool renderOperationText(GBC::Memory& memory, uint16_t address, uint16_t pc, bool* breakpoint)
   {
      ImGui::PushID(address);

      uint8_t opcode = memory.readDirect(address);
      GBC::Operation operation = GBC::kOperations[opcode];

      OperationType type = OperationType::Normal;
      if (address == pc)
      {
         type = OperationType::Current;
      }
      else if (address == pc + 1)
      {
         uint8_t previousOpcode = memory.readDirect(address - 1);
         GBC::Operation previousOperation = GBC::kOperations[previousOpcode];

         if (previousOperation.ins == GBC::Ins::PREFIX)
         {
            type = OperationType::CB;
            operation = GBC::kCBOperations[opcode];
         }
         else if (usesImm8(previousOperation) || usesImm16(previousOperation))
         {
            type = OperationType::Immediate;
         }
      }
      else if (address == pc + 2)
      {
         uint8_t previousPreviousOpcode = memory.readDirect(address - 2);
         GBC::Operation previousPreviousOperation = GBC::kOperations[previousPreviousOpcode];

         if (usesImm16(previousPreviousOperation))
         {
            type = OperationType::Immediate;
         }
      }

      std::stringstream ss;
      if (type == OperationType::Immediate)
      {
         ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(opcode);
      }
      else
      {
         const char* insText = getInsText(operation.ins);
         const char* param1Text = getOprText(operation.param1);
         const char* param2Text = getOprText(operation.param2);

         ss << insText;
         if (param1Text)
         {
            ss << " " << param1Text;
         }
         if (param2Text)
         {
            ss << ", " << param2Text;
         }
      }
      std::string text = ss.str();

      if (type != OperationType::Normal)
      {
         ImVec4 color;
         switch (type)
         {
         case OperationType::Current:
            color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            break;
         case OperationType::CB:
            color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
            break;
         case OperationType::Immediate:
            color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            break;
         default:
            break;
         }

         ImGui::PushStyleColor(ImGuiCol_Text, color);
      }

      bool breakpointToggled = ImGui::Checkbox("", breakpoint);
      ImGui::NextColumn();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("%04X: %s", address, text.c_str());
      ImGui::NextColumn();

      if (type != OperationType::Normal)
      {
         ImGui::PopStyleColor();
      }

      ImGui::Separator();

      ImGui::PopID();

      return breakpointToggled;
   }
}

void UI::renderDebuggerWindow(GBC::GameBoy& gameBoy) const
{
   ImGui::Begin("Debugger");

   bool scroll = false;
   uint16_t scrollAddress = 0;
   {
      ImGui::Columns(2);
      ImGui::SetColumnWidth(0, 100.0f);

      if (gameBoy.cpu.isInBreakMode())
      {
         bool debugContinue = ImGui::Button("Continue");
         if (debugContinue)
         {
            gameBoy.cpu.debugContinue();
         }

         bool step = ImGui::Button("Step");
         if (step)
         {
            gameBoy.cpu.step();
         }
      }
      else
      {
         bool debugBreak = ImGui::Button("Break");
         if (debugBreak)
         {
            gameBoy.cpu.debugBreak();

            scroll = true;
            scrollAddress = gameBoy.cpu.reg.pc;
         }
      }

      ImGui::NextColumn();

      {
         bool goToPc = ImGui::Button("Go to PC");
         ImGui::SameLine();

         static bool lockToPc = false;
         ImGui::Checkbox("Lock to PC", &lockToPc);

         if (goToPc || lockToPc)
         {
            scroll = true;
            scrollAddress = gameBoy.cpu.reg.pc;
         }

         ImGui::SetNextItemWidth(50.0f);
         static uint16_t targetAddress = 0;
         bool goToAddress = ImGui::InputScalar("Go to address", ImGuiDataType_U16, &targetAddress, nullptr, nullptr, "%X", ImGuiInputTextFlags_CharsHexadecimal);

         if (goToAddress)
         {
            scroll = true;
            scrollAddress = targetAddress;
         }
      }

      ImGui::Columns(1);
   }

   ImGui::Separator();

   {
      ImGui::BeginChild("##scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_NoMove);
      ImGui::Columns(2);
      ImGui::SetColumnWidth(0, 40.0f);

      ImGuiListClipper clipper(0x10000);
      while (clipper.Step())
      {
         if (clipper.StepNo == 3 && scroll)
         {
            ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + scrollAddress * clipper.ItemsHeight, 0.25f);
         }

         for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; ++line)
         {
            ASSERT(line >= 0 && line <= 0xFFFF);
            uint16_t address = static_cast<uint16_t>(line);

            static std::array<bool, 0x10000> breakpoints;
            bool breakpointToggled = renderOperationText(gameBoy.memory, address, gameBoy.cpu.reg.pc, &breakpoints[line]);

            if (breakpointToggled)
            {
               if (breakpoints[line])
               {
                  gameBoy.cpu.setBreakpoint(address);
               }
               else
               {
                  gameBoy.cpu.clearBreakpoint(address);
               }
            }
         }
      }
      clipper.End();

      ImGui::Columns(1);
      ImGui::EndChild();
   }

   ImGui::End();

   ImGui::ShowDemoWindow();
}
