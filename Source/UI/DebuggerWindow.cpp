#if !DM_WITH_DEBUGGER
#   error "Compiling debugger, but DM_WITH_DEBUGGER is not set!"
#endif // !DM_WITH_DEBUGGER

#ifdef _WIN32
#  define _CRT_SECURE_NO_WARNINGS
#endif // _WIN32

#include "UI/UI.h"

#include "UI/BeforeCoreIncludes.inl"
#  include "GameBoy/CPU.h"
#  include "GameBoy/GameBoy.h"
#  include "GameBoy/Operations.h"
#include "UI/AfterCoreIncludes.inl"

#include <imgui.h>
#include <PlatformUtils/IOUtils.h>

#include <array>
#include <cstdio>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>

namespace DotMatrix
{

namespace
{
   struct Label
   {
      std::string name;
      uint16_t address = 0;
      uint16_t size = 0;
   };

   class Symbols
   {
   public:
      static std::optional<Symbols> load(const std::string& symbolFile);

      const Label* findLabelByAddress(uint16_t address) const;
      const Label* findLabelByIndex(std::size_t index) const;

      std::size_t getNumLabels() const
      {
         return labels.size();
      }

   private:
      std::map<uint16_t, Label> labels;
   };

   // static
   std::optional<Symbols> Symbols::load(const std::string& symbolFile)
   {
      static const unsigned int kNameMaxSize = 256;

      std::optional<std::string> symbolFileData = IOUtils::readTextFile(symbolFile);
      if (!symbolFileData)
      {
         return std::nullopt;
      }
      std::string symbolFileString = std::move(*symbolFileData);

      std::size_t parseLocation = 0;
      Symbols symbols;
      std::map<std::string, uint16_t> nameToAddress;

      static const std::string kLabels = "[labels]\n";
      std::size_t labelsLocation = symbolFileString.find(kLabels);
      if (labelsLocation == std::string::npos)
      {
         return std::nullopt;
      }
      parseLocation = labelsLocation + kLabels.size() + 1;

      uint8_t bank = 0;
      uint16_t parsedAddress = 0;
      std::array<char, kNameMaxSize> parsedName{};
      while (std::sscanf(&symbolFileString[parseLocation], "%hhx:%hx %255s\n", &bank, &parsedAddress, parsedName.data()) == 3)
      {
         Label newLabel;
         newLabel.name = parsedName.data();
         newLabel.address = parsedAddress;

         bank = 0;
         parsedAddress = 0;
         parsedName[0] = '\0';

         symbols.labels.emplace(newLabel.address, newLabel);
         nameToAddress.emplace(newLabel.name, newLabel.address);

         parseLocation = symbolFileString.find('\n', parseLocation);
         if (parseLocation != std::string::npos)
         {
            ++parseLocation;
         }
      }

      static const std::string kDefinitions = "[definitions]\n";
      std::size_t definitionsLocation = symbolFileString.find("[definitions]");
      if (definitionsLocation == std::string::npos)
      {
         return std::nullopt;
      }
      parseLocation = definitionsLocation + kDefinitions.size() + 1;

      unsigned int sizeVal = 0;
      std::array<char, kNameMaxSize> parsedSizeStr{};
      while (std::sscanf(&symbolFileString[parseLocation], "%x %255s", &sizeVal, parsedSizeStr.data()) == 2)
      {
         static const std::string kSizeofPrefix = "_sizeof_";

         std::string sizeStr = parsedSizeStr.data();
         parsedSizeStr[0] = '\0';

         std::size_t sizeofLocation = sizeStr.find(kSizeofPrefix);
         if (sizeofLocation != std::string::npos)
         {
            std::string name = sizeStr.substr(sizeofLocation + kSizeofPrefix.size());

            auto nameToAddressItr = nameToAddress.find(name);
            if (nameToAddressItr != nameToAddress.end())
            {
               uint16_t address = nameToAddressItr->second;

               auto labelItr = symbols.labels.find(address);
               if (labelItr != symbols.labels.end())
               {
                  labelItr->second.size = sizeVal;
               }
            }
         }

         parseLocation = symbolFileString.find('\n', parseLocation);
         if (parseLocation != std::string::npos)
         {
            ++parseLocation;
         }
      }

      return symbols;
   }

   const Label* Symbols::findLabelByAddress(uint16_t address) const
   {
      auto itr = labels.find(address);
      if (itr != labels.end())
      {
         return &itr->second;
      }

      return nullptr;
   }

   const Label* Symbols::findLabelByIndex(std::size_t index) const
   {
      auto itr = labels.begin();
      for (std::size_t i = 0; i < index; ++i)
      {
         ++itr;
      }

      if (itr != labels.end())
      {
         return &itr->second;
      }

      return nullptr;
   }

   std::optional<Symbols> symbols;

   bool stringReplace(std::string& str, const std::string& from, const std::string& to)
   {
      std::size_t startPos = str.rfind(from);
      if (startPos == std::string::npos)
      {
         return false;
      }

      str.replace(startPos, from.length(), to);
      return true;
   }

   bool usesImm8(Operation operation)
   {
      return operation.param1 == Opr::Imm8 || operation.param2 == Opr::Imm8
         || operation.param1 == Opr::DerefImm8 || operation.param2 == Opr::DerefImm8
         || operation.param1 == Opr::Imm8Signed || operation.param2 == Opr::Imm8Signed;
   }

   bool usesImm16(Operation operation)
   {
      return operation.param1 == Opr::Imm16 || operation.param2 == Opr::Imm16
         || operation.param1 == Opr::DerefImm16 || operation.param2 == Opr::DerefImm16;
   }

   const char* getInsText(Ins ins)
   {
      switch (ins)
      {
      case Ins::Invalid: return "Invalid";

      case Ins::LD: return "LD";
      case Ins::LDD: return "LDD";
      case Ins::LDI: return "LDI";
      case Ins::LDH: return "LDH";
      case Ins::LDHL: return "LDHL";
      case Ins::PUSH: return "PUSH";
      case Ins::POP: return "POP";

      case Ins::ADD: return "ADD";
      case Ins::ADC: return "ADC";
      case Ins::SUB: return "SUB";
      case Ins::SBC: return "SBC";
      case Ins::AND: return "AND";
      case Ins::OR: return "OR";
      case Ins::XOR: return "XOR";
      case Ins::CP: return "CP";
      case Ins::INC: return "INC";
      case Ins::DEC: return "DEC";

      case Ins::SWAP: return "SWAP";
      case Ins::DAA: return "DAA";
      case Ins::CPL: return "CPL";
      case Ins::CCF: return "CCF";
      case Ins::SCF: return "SCF";
      case Ins::NOP: return "NOP";
      case Ins::HALT: return "HALT";
      case Ins::STOP: return "STOP";
      case Ins::DI: return "DI";
      case Ins::EI: return "EI";

      case Ins::RLCA: return "RLCA";
      case Ins::RLA: return "RLA";
      case Ins::RRCA: return "RRCA";
      case Ins::RRA: return "RRA";
      case Ins::RLC: return "RLC";
      case Ins::RL: return "RL";
      case Ins::RRC: return "RRC";
      case Ins::RR: return "RR";
      case Ins::SLA: return "SLA";
      case Ins::SRA: return "SRA";
      case Ins::SRL: return "SRL";

      case Ins::BIT: return "BIT";
      case Ins::SET: return "SET";
      case Ins::RES: return "RES";

      case Ins::JP: return "JP";
      case Ins::JR: return "JR";

      case Ins::CALL: return "CALL";

      case Ins::RST: return "RST";

      case Ins::RET: return "RET";
      case Ins::RETI: return "RETI";

      case Ins::PREFIX: return "PREFIX CB";

      default: return "Unknown";
      }
   }

   const char* getOprText(Opr opr)
   {
      switch (opr)
      {
      case Opr::None: return nullptr;

      case Opr::A: return "A";
      case Opr::F: return "F";
      case Opr::B: return "B";
      case Opr::C: return "C";
      case Opr::D: return "D";
      case Opr::E: return "E";
      case Opr::H: return "H";
      case Opr::L: return "L";

      case Opr::AF: return "AF";
      case Opr::BC: return "BC";
      case Opr::DE: return "DE";
      case Opr::HL: return "HL";
      case Opr::SP: return "SP";
      case Opr::PC: return "PC";

      case Opr::Imm8: return "d8";
      case Opr::Imm16: return "d16";
      case Opr::Imm8Signed: return "r8";

      case Opr::DerefC: return "(C)";

      case Opr::DerefBC: return "(BC)";
      case Opr::DerefDE: return "(DE)";
      case Opr::DerefHL: return "(HL)";

      case Opr::DerefImm8: return "(a8)";
      case Opr::DerefImm16: return "(a16)";

      case Opr::FlagC: return "C";
      case Opr::FlagNC: return "NC";
      case Opr::FlagZ: return "Z";
      case Opr::FlagNZ: return "NZ";

      case Opr::Bit0: return "0";
      case Opr::Bit1: return "1";
      case Opr::Bit2: return "2";
      case Opr::Bit3: return "3";
      case Opr::Bit4: return "4";
      case Opr::Bit5: return "5";
      case Opr::Bit6: return "6";
      case Opr::Bit7: return "7";

      case Opr::Rst00H: return "00H";
      case Opr::Rst08H: return "08H";
      case Opr::Rst10H: return "10H";
      case Opr::Rst18H: return "18H";
      case Opr::Rst20H: return "20H";
      case Opr::Rst28H: return "28H";
      case Opr::Rst30H: return "30H";
      case Opr::Rst38H: return "38H";

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

   bool renderOperation(GameBoy& gameBoy, uint16_t address, uint16_t pc, bool* breakpoint)
   {
      ImGui::PushID(address);

      uint8_t opcode = gameBoy.readDirect(address);
      Operation operation = kOperations[opcode];

      OperationType type = OperationType::Normal;
      if (address == pc)
      {
         type = OperationType::Current;
      }
      else if (address == pc + 1)
      {
         uint8_t previousOpcode = gameBoy.readDirect(address - 1);
         Operation previousOperation = kOperations[previousOpcode];

         if (previousOperation.ins == Ins::PREFIX)
         {
            type = OperationType::CB;
            operation = kCBOperations[opcode];
         }
         else if (usesImm8(previousOperation) || usesImm16(previousOperation))
         {
            type = OperationType::Immediate;
         }
      }
      else if (address == pc + 2)
      {
         uint8_t previousPreviousOpcode = gameBoy.readDirect(address - 2);
         Operation previousPreviousOperation = kOperations[previousPreviousOpcode];

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

      bool breakpointToggled = ImGui::Checkbox("", breakpoint);
      ImGui::NextColumn();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("%04X", address);
      ImGui::NextColumn();

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

      ImGui::AlignTextToFramePadding();
      ImGui::Text("%s", text.c_str());
      ImGui::NextColumn();

      if (type != OperationType::Normal)
      {
         ImGui::PopStyleColor();
      }

      if (symbols)
      {
         if (const Label* label = symbols->findLabelByAddress(address))
         {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("[%s]", label->name.c_str());
         }
         ImGui::NextColumn();
      }

      ImGui::Separator();

      ImGui::PopID();

      return breakpointToggled;
   }

   std::array<bool, 0x10000> breakpoints;

   void updateBreakpoint(GameBoy& gameBoy, uint16_t address)
   {
      if (breakpoints[address])
      {
         gameBoy.setBreakpoint(address);
      }
      else
      {
         gameBoy.clearBreakpoint(address);
      }
   }
}

void UI::renderDebuggerWindow(GameBoy& gameBoy) const
{
   ImGui::SetNextWindowPos(ImVec2(1006.0f, 190.0f), ImGuiCond_FirstUseEver);
   ImGui::SetNextWindowSize(ImVec2(429.0f, 390.0f), ImGuiCond_FirstUseEver);
   ImGui::Begin("Debugger");

   bool scroll = false;
   uint16_t scrollAddress = 0;
   {
      ImGui::Columns(2);
      ImGui::SetColumnWidth(0, 93.0f);

      if (gameBoy.isInBreakMode())
      {
         bool debugContinue = ImGui::Button("Continue");
         if (debugContinue)
         {
            gameBoy.debugContinue();
         }

         bool step = ImGui::Button("Step");
         if (step)
         {
            gameBoy.debugStep();
         }
      }
      else
      {
         bool debugBreak = ImGui::Button("Break");
         if (debugBreak)
         {
            gameBoy.debugBreak();

            scroll = true;
            scrollAddress = gameBoy.cpu.reg.pc;
         }
      }

      ImGui::NextColumn();

      {
         bool goToPc = ImGui::Button("Go to PC");
         ImGui::SameLine();

         static bool lockToPc = false;
         ImGui::Checkbox("Lock", &lockToPc);

         if (goToPc || lockToPc)
         {
            scroll = true;
            scrollAddress = gameBoy.cpu.reg.pc;
         }

         bool goToAddress = ImGui::Button("Go to address");
         ImGui::SameLine();
         ImGui::SetNextItemWidth(50.0f);
         static uint16_t targetAddress = 0;
         goToAddress |= ImGui::InputScalar("##address", ImGuiDataType_U16, &targetAddress, nullptr, nullptr, "%X", ImGuiInputTextFlags_CharsHexadecimal);

         if (goToAddress)
         {
            scroll = true;
            scrollAddress = targetAddress;
         }

         if (symbols)
         {
            static int currentLabel = 0;
            bool goToLabel = ImGui::Button("Go to label");
            ImGui::SameLine();
            goToLabel |= ImGui::Combo("##label", &currentLabel, [](void* data, int idx, const char** out_text)
            {
               Symbols& symbolsData = *reinterpret_cast<Symbols*>(data);

               if (const Label* label = symbolsData.findLabelByIndex(idx))
               {
                  *out_text = label->name.data();
                  return true;
               }

               return false;
            }, static_cast<void*>(&symbols.value()), static_cast<int>(symbols->getNumLabels()));

            if (goToLabel)
            {
               if (const Label* label = symbols->findLabelByIndex(currentLabel))
               {
                  scroll = true;
                  scrollAddress = label->address;
               }
            }
         }
      }

      ImGui::Columns(1);
   }

   {
      ImGui::Columns(symbols ? 4 : 3);
      ImGui::SetColumnWidth(0, 48.0f);
      ImGui::SetColumnWidth(1, 45.0f);
      ImGui::SetColumnWidth(2, 125.0f);

      ImGui::Separator();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Break");
      ImGui::NextColumn();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Addr");
      ImGui::NextColumn();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Instruction");
      ImGui::NextColumn();

      if (symbols)
      {
         ImGui::AlignTextToFramePadding();
         ImGui::Text("Label");
         ImGui::NextColumn();
      }

      ImGui::Columns(1);
   }

   ImGui::Separator();

   {
      ImGui::BeginChild("##scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_NoMove);
      ImGui::Columns(symbols ? 4 : 3);
      ImGui::SetColumnWidth(0, 40.0f);
      ImGui::SetColumnWidth(1, 45.0f);
      ImGui::SetColumnWidth(2, 125.0f);

      ImGuiListClipper clipper;
      clipper.Begin(0x10000);
      while (clipper.Step())
      {
         if (clipper.StepNo == 3 && scroll)
         {
            ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + scrollAddress * clipper.ItemsHeight, 0.25f);
         }

         for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; ++line)
         {
            DM_ASSERT(line >= 0 && line <= 0xFFFF);
            uint16_t address = static_cast<uint16_t>(line);

            bool breakpointToggled = renderOperation(gameBoy, address, gameBoy.cpu.reg.pc, &breakpoints[line]);
            if (breakpointToggled)
            {
               updateBreakpoint(gameBoy, address);
            }
         }
      }
      clipper.End();

      ImGui::Columns(1);
      ImGui::EndChild();
   }

   ImGui::End();
}

void UI::onRomLoaded_Debugger(GameBoy& gameBoy, const char* romPath) const
{
   std::string symbolPath = romPath;
   if (stringReplace(symbolPath, ".gb", ".sym"))
   {
      symbols = Symbols::load(symbolPath);
   }
   else
   {
      symbols = std::nullopt;
   }

   for (std::size_t i = 0; i < breakpoints.size(); ++i)
   {
      updateBreakpoint(gameBoy, static_cast<uint16_t>(i));
   }

   if (gameBoy.shouldBreak())
   {
      gameBoy.debugBreak();
   }
}

} // namespace DotMatrix
