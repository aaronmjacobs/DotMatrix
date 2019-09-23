#include "UI/UI.h"

#include "Emulator.h"

#include "Core/Enum.h"
#include "Core/Math.h"

#include "GBC/Cartridge.h"
#include "GBC/GameBoy.h"
#include "GBC/LCDController.h"

#include "Platform/Video/Renderer.h"

#include <glad/glad.h>

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include <cmath>
#include <sstream>

namespace
{

const uint64_t kZero = 0;
const int kNumPlottedSamples = GBC::SoundController::kSampleRate / 4;

struct MemoryHelper
{
   GBC::Memory* memory = nullptr;
   uint16_t address = 0;
};

MemoryEditor::u8 readMemory(const MemoryEditor::u8* data, std::size_t offset)
{
   const MemoryHelper* memoryHelper = reinterpret_cast<const MemoryHelper*>(data);

   return memoryHelper->memory->readDirect(memoryHelper->address + static_cast<uint16_t>(offset));
}

void writeMemory(MemoryEditor::u8* data, std::size_t offset, MemoryEditor::u8 value)
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

void updateSamples(std::vector<float>& samples, int& offset, const std::vector<int8_t>& audioData)
{
   for (int8_t audioSample : audioData)
   {
      samples[offset] = static_cast<float>(audioSample);
      offset = (offset + 1) % kNumPlottedSamples;
   }
}

std::string getPitch(uint32_t timerPeriod)
{
   static const uint32_t kA4 = 440;
   static const uint32_t kC0 = Math::round<uint32_t>(kA4 * std::pow(2, -4.75));
   static const std::array<const char*, 12> kNames = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

   if (timerPeriod == 0)
   {
      return "";
   }

   double frequency = GBC::CPU::kClockSpeed / static_cast<double>(timerPeriod);
   uint32_t halfSteps = Math::round<uint32_t>(12 * std::log2(frequency / kC0));
   uint32_t octave = halfSteps / 12;
   uint32_t pitch = halfSteps % 12;

   std::stringstream ss;
   ss << kNames[pitch] << " " << octave;
   return ss.str();
}

const char* getCartridgeTypeText(GBC::Cartridge::Type type)
{
   switch (type)
   {
   case GBC::Cartridge::Type::ROM:
      return "ROM";

   case GBC::Cartridge::Type::MBC1:
      return "MBC1";
   case GBC::Cartridge::Type::MBC1PlusRAM:
      return "MBC1 + RAM";
   case GBC::Cartridge::Type::MBC1PlusRAMPlusBattery:
      return "MBC1 + RAM + Battery";

   case GBC::Cartridge::Type::MBC2:
      return "MBC2";
   case GBC::Cartridge::Type::MBC2PlusBattery:
      return "MBC2 + Battery";

   case GBC::Cartridge::Type::ROMPlusRAM:
      return "ROM + RAM";
   case GBC::Cartridge::Type::ROMPlusRAMPlusBattery:
      return "ROM + RAM + Battery";

   case GBC::Cartridge::Type::MMM01:
      return "MM01";
   case GBC::Cartridge::Type::MMM01PlusRAM:
      return "MM01 + RAM";
   case GBC::Cartridge::Type::MMM01PlusRAMPlusBattery:
      return "MM01 + RAM + Battery";

   case GBC::Cartridge::Type::MBC3PlusTimerPlusBattery:
      return "MBC3 + Timer + Battery";
   case GBC::Cartridge::Type::MBC3PlusTimerPlusRAMPlusBattery:
      return "MBC3 + Timer + RAM + Battery";
   case GBC::Cartridge::Type::MBC3:
      return "MBC3";
   case GBC::Cartridge::Type::MBC3PlusRAM:
      return "MBC3 + RAM";
   case GBC::Cartridge::Type::MBC3PlusRAMPlusBattery:
      return "MBC3 + RAM + Battery";

   case GBC::Cartridge::Type::MBC4:
      return "MBC4";
   case GBC::Cartridge::Type::MBC4PlusRAM:
      return "MBC4 + RAM";
   case GBC::Cartridge::Type::MBC4PlusRAMPlusBattery:
      return "MBC4 + RAM + Battery";

   case GBC::Cartridge::Type::MBC5:
      return "MBC5";
   case GBC::Cartridge::Type::MBC5PlusRAM:
      return "MBC5 + RAM";
   case GBC::Cartridge::Type::MBC5PlusRAMPlusBattery:
      return "MBC5 + RAM + Battery";
   case GBC::Cartridge::Type::MBC5PlusRumble:
      return "MBC5 + Rumble";
   case GBC::Cartridge::Type::MBC5PlusRumblePlusRAM:
      return "MBC5 + Rumble + RAM";
   case GBC::Cartridge::Type::MBC5PlusRumblePlusRAMPlusBattery:
      return "MBC5 + Rumble + RAM + Battery";

   case GBC::Cartridge::Type::MBC6:
      return "MBC6";

   case GBC::Cartridge::Type::MBC7PlusSensorPlusRumblePlusRAMPlusBattery:
      return "MBC7 + Sensor + Rumble + RAM + Battery";

   case GBC::Cartridge::Type::PocketCamera:
      return "Pocket Camera";
   case GBC::Cartridge::Type::BandaiTAMA5:
      return "Bandai TAMA 5";
   case GBC::Cartridge::Type::HuC3:
      return "HuC3";
   case GBC::Cartridge::Type::HuC1PlusRAMPlusBattery:
      return "HuC1 + RAM + Battery";
   default:
      return "Unknown";
   }
}

const char* getROMSizeText(GBC::Cartridge::ROMSize romSize)
{
   switch (romSize)
   {
   case GBC::Cartridge::ROMSize::Size32KBytes:
      return "32 KB (no banking)";
   case GBC::Cartridge::ROMSize::Size64KBytes:
      return "64 KB (4 banks)";
   case GBC::Cartridge::ROMSize::Size128KBytes:
      return "128 KB (8 banks)";
   case GBC::Cartridge::ROMSize::Size256KBytes:
      return "256 KB (16 banks)";
   case GBC::Cartridge::ROMSize::Size512KBytes:
      return "512 KB (32 banks)";
   case GBC::Cartridge::ROMSize::Size1MBytes:
      return "1 MB (64 banks)";
   case GBC::Cartridge::ROMSize::Size2MBytes:
      return "2 MB (128 banks)";
   case GBC::Cartridge::ROMSize::Size4MBytes:
      return "4 MB (256 banks)";
   case GBC::Cartridge::ROMSize::Size1Point1MBytes:
      return "1.1 MB (72 banks)";
   case GBC::Cartridge::ROMSize::Size1Point2MBytes:
      return "1.2 MB (80 banks)";
   case GBC::Cartridge::ROMSize::Size1Point5MBytes:
      return "1.5 MB (96 banks)";
   default:
      return "Unknown";
   }
}

const char* getRAMSizeText(GBC::Cartridge::RAMSize ramSize)
{
   switch (ramSize)
   {
   case GBC::Cartridge::RAMSize::None:
      return "0 KB (no RAM)";
   case GBC::Cartridge::RAMSize::Size2KBytes:
      return "2 KB (1 bank)";
   case GBC::Cartridge::RAMSize::Size8KBytes:
      return "8 KB (1 bank)";
   case GBC::Cartridge::RAMSize::Size32KBytes:
      return "32 KB (4 banks)";
   case GBC::Cartridge::RAMSize::Size128KBytes:
      return "128 KB (16 banks)";
   case GBC::Cartridge::RAMSize::Size64KBytes:
      return "64 KB (8 banks)";
   default:
      return "Unknown";
   }
}

const char* getLicenseeText(uint8_t oldCode, std::array<uint8_t, 2> newCode)
{
   switch (oldCode)
   {
   case 0x00: return "none";
   case 0x01: return "nintendo";
   case 0x08: return "capcom";
   case 0x09: return "hot-b";
   case 0x0A: return "jaleco";
   case 0x0B: return "coconuts";
   case 0x0C: return "elite systems";
   case 0x13: return "electronic arts";
   case 0x18: return "hudsonsoft";
   case 0x19: return "itc entertainment";
   case 0x1A: return "yanoman";
   case 0x1D: return "clary";
   case 0x1F: return "virgin";
   case 0x20: return "KSS";
   case 0x24: return "pcm complete";
   case 0x25: return "san-x";
   case 0x28: return "kotobuki systems";
   case 0x29: return "seta";
   case 0x30: return "infogrames";
   case 0x31: return "nintendo";
   case 0x32: return "bandai";
   case 0x33: break; // Use new code
   case 0x34: return "konami";
   case 0x35: return "hector";
   case 0x38: return "Capcom";
   case 0x39: return "Banpresto";
   case 0x3C: return "entertainment i";
   case 0x3E: return "gremlin";
   case 0x41: return "Ubisoft";
   case 0x42: return "Atlus";
   case 0x44: return "Malibu";
   case 0x46: return "angel";
   case 0x47: return "spectrum holoby";
   case 0x49: return "irem";
   case 0x4A: return "virgin";
   case 0x4D: return "malibu";
   case 0x4F: return "u.s. gold";
   case 0x50: return "absolute";
   case 0x51: return "acclaim";
   case 0x52: return "activision";
   case 0x53: return "american sammy";
   case 0x54: return "gametek";
   case 0x55: return "park place";
   case 0x56: return "ljn";
   case 0x57: return "matchbox";
   case 0x59: return "milton bradley";
   case 0x5A: return "mindscape";
   case 0x5B: return "romstar";
   case 0x5C: return "naxat soft";
   case 0x5D: return "tradewest";
   case 0x60: return "titus";
   case 0x61: return "virgin";
   case 0x67: return "ocean";
   case 0x69: return "electronic arts";
   case 0x6E: return "elite systems";
   case 0x6F: return "electro brain";
   case 0x70: return "Infogrammes";
   case 0x71: return "Interplay";
   case 0x72: return "broderbund";
   case 0x73: return "sculptered soft";
   case 0x75: return "the sales curve";
   case 0x78: return "thq";
   case 0x79: return "accolade";
   case 0x7A: return "triffix entertainment";
   case 0x7C: return "microprose";
   case 0x7F: return "kemco";
   case 0x80: return "misawa entertainment";
   case 0x83: return "lozc";
   case 0x86: return "tokuma shoten intermedia";
   case 0x8B: return "bullet-proof software";
   case 0x8C: return "vic tokai";
   case 0x8E: return "ape";
   case 0x8F: return "i'max";
   case 0x91: return "chun soft";
   case 0x92: return "video system";
   case 0x93: return "tsuburava";
   case 0x95: return "varie";
   case 0x96: return "yonezawa/s'pal";
   case 0x97: return "kaneko";
   case 0x99: return "arc";
   case 0x9A: return "nihon bussan";
   case 0x9B: return "Tecmo";
   case 0x9C: return "imagineer";
   case 0x9D: return "Banpresto";
   case 0x9F: return "nova";
   case 0xA1: return "Hori electric";
   case 0xA2: return "Bandai";
   case 0xA4: return "Konami";
   case 0xA6: return "kawada";
   case 0xA7: return "takara";
   case 0xA9: return "technos japan";
   case 0xAA: return "broderbund";
   case 0xAC: return "Toei animation";
   case 0xAD: return "toho";
   case 0xAF: return "Namco";
   case 0xB0: return "Acclaim";
   case 0xB1: return "ascii or nexoft";
   case 0xB2: return "Bandai";
   case 0xB4: return "Enix";
   case 0xB6: return "HAL";
   case 0xB7: return "SNK";
   case 0xB9: return "pony canyon";
   case 0xBA: return "culture brain o";
   case 0xBB: return "Sunsoft";
   case 0xBD: return "Sony imagesoft";
   case 0xBF: return "sammy";
   case 0xC0: return "Taito";
   case 0xC2: return "Kemco";
   case 0xC3: return "Squaresoft";
   case 0xC4: return "tokuma shoten intermedia";
   case 0xC5: return "data east";
   case 0xC6: return "tonkin house";
   case 0xC8: return "koei";
   case 0xC9: return "ufl";
   case 0xCA: return "ultra";
   case 0xCB: return "vap";
   case 0xCC: return "use";
   case 0xCD: return "meldac";
   case 0xCE: return "pony canyon or";
   case 0xCF: return "angel";
   case 0xD0: return "Taito";
   case 0xD1: return "sofel";
   case 0xD2: return "quest";
   case 0xD3: return "sigma enterprises";
   case 0xD4: return "ask kodansha";
   case 0xD6: return "naxat soft";
   case 0xD7: return "copya systems";
   case 0xD9: return "Banpresto";
   case 0xDA: return "tomy";
   case 0xDB: return "ljn";
   case 0xDD: return "ncs";
   case 0xDE: return "human";
   case 0xDF: return "altron";
   case 0xE0: return "jaleco";
   case 0xE1: return "towachiki";
   case 0xE2: return "uutaka";
   case 0xE3: return "varie";
   case 0xE5: return "epoch";
   case 0xE7: return "athena";
   case 0xE8: return "asmik";
   case 0xE9: return "natsume";
   case 0xEA: return "king records";
   case 0xEB: return "atlus";
   case 0xEC: return "Epic/Sony records";
   case 0xEE: return "igs";
   case 0xF0: return "a wave";
   case 0xF3: return "extreme entertainment";
   case 0xFF: return "ljn";
   default: return "Unknown";
   }

   uint8_t combinedNewCode = ((newCode[0] & 0x0F) << 4) + (newCode[1] & 0x0F);
   switch (combinedNewCode)
   {
   case 0x00: return "none";
   case 0x01: return "nintendo";
   case 0x08: return "capcom";
   case 0x13: return "electronic arts";
   case 0x18: return "hudsonsoft";
   case 0x19: return "b-ai";
   case 0x20: return "kss";
   case 0x22: return "pow";
   case 0x24: return "pcm complete";
   case 0x25: return "san-x";
   case 0x28: return "kemco japan";
   case 0x29: return "seta";
   case 0x30: return "viacom";
   case 0x31: return "nintendo";
   case 0x32: return "bandia";
   case 0x33: return "ocean/acclaim";
   case 0x34: return "konami";
   case 0x35: return "hector";
   case 0x37: return "taito";
   case 0x38: return "hudson";
   case 0x39: return "banpresto";
   case 0x41: return "ubi soft";
   case 0x42: return "atlus";
   case 0x44: return "malibu";
   case 0x46: return "angel";
   case 0x47: return "pullet-proof";
   case 0x49: return "irem";
   case 0x50: return "absolute";
   case 0x51: return "acclaim";
   case 0x52: return "activision";
   case 0x53: return "american sammy";
   case 0x54: return "konami";
   case 0x55: return "hi tech entertainment";
   case 0x56: return "ljn";
   case 0x57: return "matchbox";
   case 0x58: return "mattel";
   case 0x59: return "milton bradley";
   case 0x60: return "titus";
   case 0x61: return "virgin";
   case 0x64: return "lucasarts";
   case 0x67: return "ocean";
   case 0x69: return "electronic arts";
   case 0x70: return "infogrames";
   case 0x71: return "interplay";
   case 0x72: return "broderbund";
   case 0x73: return "sculptured";
   case 0x75: return "sci";
   case 0x78: return "thq";
   case 0x79: return "accolade";
   case 0x80: return "misawa";
   case 0x83: return "lozc";
   case 0x86: return "tokuma shoten i";
   case 0x87: return "tsukuda ori";
   case 0x91: return "chun soft";
   case 0x92: return "video system";
   case 0x93: return "ocean/acclaim";
   case 0x95: return "varie";
   case 0x96: return "yonezawa/s'pal";
   case 0x97: return "kaneko";
   case 0x99: return "pack in soft";
   default: return "Unknown";
   }
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

void UI::render(Emulator& emulator)
{
   ImGui_ImplOpenGL3_NewFrame();
   ImGui_ImplGlfw_NewFrame();
   ImGui::NewFrame();

   renderScreen(*emulator.renderer);
   renderEmulator(emulator);
   renderJoypad(*emulator.gameBoy);
   renderCPU(*emulator.gameBoy);
   renderMemory(*emulator.gameBoy);
   renderSoundController(*emulator.gameBoy);
   renderCartridge(*emulator.gameBoy);

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

void UI::renderEmulator(Emulator& emulator) const
{
   ImGui::Begin("Emulator");

   float timeScale = static_cast<float>(emulator.timeScale);
   ImGui::SliderFloat("Time scale", &timeScale, 0.0f, 1.0f, nullptr, 2.0f);
   emulator.timeScale = timeScale;

   uint64_t clockSpeed = Math::round<uint64_t>(GBC::CPU::kClockSpeed * timeScale);
   ImGui::Text("Clock speed: %llu", clockSpeed);

   if (ImGui::Button("Step"))
   {
      emulator.timeScale = 0.0;
      emulator.gameBoy->cpu.tick();
   }

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
   ImGui::CheckboxFlags("Zero", flags, Enum::cast(GBC::CPU::Flag::Zero));
   ImGui::CheckboxFlags("Subtract", flags, Enum::cast(GBC::CPU::Flag::Sub));
   ImGui::CheckboxFlags("Half Carry", flags, Enum::cast(GBC::CPU::Flag::HalfCarry));
   ImGui::CheckboxFlags("Carry", flags, Enum::cast(GBC::CPU::Flag::Carry));
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

   GBC::SoundController& soundController = gameBoy.getSoundController();

   if (ImGui::CollapsingHeader("Output", ImGuiTreeNodeFlags_DefaultOpen))
   {
      bool powerEnabled = soundController.powerEnabled;
      ImGui::Checkbox("Power enabled", &powerEnabled);
      soundController.setPowerEnabled(powerEnabled);

      static std::vector<float> leftSamples(kNumPlottedSamples);
      static std::vector<float> rightSamples(kNumPlottedSamples);
      static int offset = 0;

      const std::vector<GBC::AudioSample>& audioData = soundController.getAudioData();
      for (const GBC::AudioSample& audioSample : audioData)
      {
         leftSamples[offset] = static_cast<float>(audioSample.left);
         rightSamples[offset] = static_cast<float>(audioSample.right);
         offset = (offset + 1) % kNumPlottedSamples;
      }

      ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 50.0f);
      ImGui::PlotLines("Left", audioSampleGetter, &leftSamples, kNumPlottedSamples, offset, nullptr, SHRT_MIN, SHRT_MAX, ImVec2(0.0f, 100.0f));
      ImGui::PlotLines("Right", audioSampleGetter, &rightSamples, kNumPlottedSamples, offset, nullptr, SHRT_MIN, SHRT_MAX, ImVec2(0.0f, 100.0f));
      ImGui::PopItemWidth();
   }

   if (ImGui::CollapsingHeader("Mixer"))
   {
      GBC::Mixer& mixer = soundController.mixer;

      ImGui::Columns(2, "mixer");

      ImGui::Checkbox("Vin left enabled", &mixer.vinLeftEnabled);
      ImGui::NextColumn();
      ImGui::Checkbox("Vin right enabled", &mixer.vinRightEnabled);
      ImGui::NextColumn();

      ImGui::Checkbox("Square 1 left enabled", &mixer.square1LeftEnabled);
      ImGui::NextColumn();
      ImGui::Checkbox("Square 1 right enabled", &mixer.square1RightEnabled);
      ImGui::NextColumn();

      ImGui::Checkbox("Square 2 left enabled", &mixer.square2LeftEnabled);
      ImGui::NextColumn();
      ImGui::Checkbox("Square 2 right enabled", &mixer.square2RightEnabled);
      ImGui::NextColumn();

      ImGui::Checkbox("Wave left enabled", &mixer.waveLeftEnabled);
      ImGui::NextColumn();
      ImGui::Checkbox("Wave right enabled", &mixer.waveRightEnabled);
      ImGui::NextColumn();

      ImGui::Checkbox("Noise left enabled", &mixer.noiseLeftEnabled);
      ImGui::NextColumn();
      ImGui::Checkbox("Noise right enabled", &mixer.noiseRightEnabled);
      ImGui::NextColumn();

      uint8_t volumeMin = 0x01;
      uint8_t volumeMax = 0x08;
      ImGui::SliderScalar("Left volume", ImGuiDataType_U8, &mixer.leftVolume, &volumeMin, &volumeMax, "0x%02X");
      ImGui::NextColumn();
      ImGui::SliderScalar("Right volume", ImGuiDataType_U8, &mixer.rightVolume, &volumeMin, &volumeMax, "0x%02X");
      ImGui::NextColumn();

      ImGui::Columns(1);
   }

   static const uint32_t kWaveMaxPeriod = 8192;

   if (ImGui::CollapsingHeader("Square Wave Channel 1"))
   {
      ImGui::PushID("Square Wave Channel 1");

      GBC::SquareWaveChannel& squareWaveChannel1 = soundController.squareWaveChannel1;

      static std::vector<float> samples(kNumPlottedSamples);
      static int offset = 0;
      updateSamples(samples, offset, soundController.getSquare1Data());

      renderSoundChannel(squareWaveChannel1, samples, offset);
      renderSoundTimer(squareWaveChannel1.timer, kWaveMaxPeriod, true);
      renderDutyUnit(squareWaveChannel1.dutyUnit);
      renderLengthUnit(squareWaveChannel1.lengthUnit);
      renderEnvelopeUnit(squareWaveChannel1.envelopeUnit);
      renderSweepUnit(squareWaveChannel1.sweepUnit);

      ImGui::PopID();
   }

   if (ImGui::CollapsingHeader("Square Wave Channel 2"))
   {
      ImGui::PushID("Square Wave Channel 2");

      GBC::SquareWaveChannel& squareWaveChannel2 = soundController.squareWaveChannel2;

      static std::vector<float> samples(kNumPlottedSamples);
      static int offset = 0;
      updateSamples(samples, offset, soundController.getSquare2Data());

      renderSoundChannel(squareWaveChannel2, samples, offset);
      renderSoundTimer(squareWaveChannel2.timer, kWaveMaxPeriod, true);
      renderDutyUnit(squareWaveChannel2.dutyUnit);
      renderLengthUnit(squareWaveChannel2.lengthUnit);
      renderEnvelopeUnit(squareWaveChannel2.envelopeUnit);

      ImGui::PopID();
   }

   if (ImGui::CollapsingHeader("Wave Channel"))
   {
      ImGui::PushID("Wave Channel");

      GBC::WaveChannel& waveChannel = soundController.waveChannel;

      static std::vector<float> samples(kNumPlottedSamples);
      static int offset = 0;
      updateSamples(samples, offset, soundController.getWaveData());

      renderSoundChannel(waveChannel, samples, offset);
      renderSoundTimer(waveChannel.timer, kWaveMaxPeriod, true);
      renderLengthUnit(waveChannel.lengthUnit);
      renderWaveUnit(waveChannel.waveUnit);

      ImGui::PopID();
   }

   if (ImGui::CollapsingHeader("Noise Channel"))
   {
      ImGui::PushID("Noise Channel");

      static const uint32_t kMaxNoisePeriod = 112 << 0x0F;

      GBC::NoiseChannel& noiseChannel = soundController.noiseChannel;

      static std::vector<float> samples(kNumPlottedSamples);
      static int offset = 0;
      updateSamples(samples, offset, soundController.getNoiseData());

      renderSoundChannel(noiseChannel, samples, offset);
      renderSoundTimer(noiseChannel.timer, kMaxNoisePeriod, false);
      renderLengthUnit(noiseChannel.lengthUnit);
      renderEnvelopeUnit(noiseChannel.envelopeUnit);
      renderLFSRUnit(noiseChannel.lfsrUnit);

      ImGui::PopID();
   }

   ImGui::End();
}

void UI::renderCartridge(GBC::GameBoy& gameBoy) const
{
   ImGui::Begin("Cartridge");

   if (gameBoy.cart == nullptr)
   {
      ImGui::Text("Please insert a cartridge");
      ImGui::End();
      return;
   }

   if (ImGui::CollapsingHeader("Header", ImGuiTreeNodeFlags_DefaultOpen))
   {
      GBC::Cartridge::Header header = gameBoy.cart->header;

      ImGui::Columns(2, "header");
      ImGui::Separator();

      ImGui::Text("Title: %s", gameBoy.cart->title());
      ImGui::NextColumn();

      ImGui::Text("Type: %s", getCartridgeTypeText(header.type));
      ImGui::NextColumn();

      ImGui::Text("Licensee: %s", getLicenseeText(header.oldLicenseeCode, header.newLicenseeCode));
      ImGui::NextColumn();

      ImGui::Text("ROM Size: %s", getROMSizeText(header.romSize));
      ImGui::NextColumn();

      ImGui::Text("Version: %d", header.maskRomVersionNumber);
      ImGui::NextColumn();

      ImGui::Text("RAM Size: %s", getRAMSizeText(header.ramSize));
      ImGui::NextColumn();

      const char* destinationText = header.destinationCode == GBC::Cartridge::DestinationCode::Japanese ? "Japanese" : "Non-Japanese";
      ImGui::Text("Destination: %s", destinationText);
      ImGui::NextColumn();

      ImGui::Text("Header Checksum: 0x%02X", header.headerChecksum);
      ImGui::NextColumn();

      const char* cgbFlagText = nullptr;
      switch (header.cgbFlag)
      {
      case GBC::Cartridge::CGBFlag::Ignored:
         cgbFlagText = "Ignored";
         break;
      case GBC::Cartridge::CGBFlag::Supported:
         cgbFlagText = "Supported";
         break;
      case GBC::Cartridge::CGBFlag::Required:
         cgbFlagText = "Required";
         break;
      default:
         cgbFlagText = "Unknown";
         break;
      }
      ImGui::Text("CGB: %s", cgbFlagText);
      ImGui::NextColumn();

      ImGui::Text("Global Checksum: 0x%02X%02X", header.globalChecksum[0], header.globalChecksum[1]);
      ImGui::NextColumn();

      const char* sgbFlagText = nullptr;
      switch (header.sgbFlag)
      {
      case GBC::Cartridge::SGBFlag::Ignored:
         sgbFlagText = "Ignored";
         break;
      case GBC::Cartridge::SGBFlag::Supported:
         sgbFlagText = "Supported";
         break;
      default:
         sgbFlagText = "Unknown";
         break;
      }
      ImGui::Text("SGB: %s", sgbFlagText);
      ImGui::NextColumn();

      bool supportsGBC = (Enum::cast(header.cgbFlag) & Enum::cast(GBC::Cartridge::CGBFlag::Supported)) != 0x00;
      if (supportsGBC)
      {
         ImGui::Text("Manufacturer Code: 0x%02X%02X%02X%02X", header.manufacturerCode[0], header.manufacturerCode[1], header.manufacturerCode[2], header.manufacturerCode[3]);
      }
      else
      {
         ImGui::Text("Manufacturer Code: None");
      }
      ImGui::NextColumn();

      ImGui::Columns(1);
      ImGui::Separator();
   }

   ImGui::End();
}

void UI::renderSoundChannel(GBC::SoundChannel& soundChannel, std::vector<float>& samples, int offset) const
{
   if (ImGui::TreeNode("Channel"))
   {
      ImGui::Checkbox("Enabled", &soundChannel.enabled);

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50.0f);
      ImGui::PlotLines("Output", audioSampleGetter, &samples, kNumPlottedSamples, offset, nullptr, -16.0f, 15.0f, ImVec2(0.0f, 100.0f));

      ImGui::TreePop();
   }
}

template<typename T>
void UI::renderSoundTimer(GBC::SoundTimer<T>& soundTimer, uint32_t maxPeriod, bool displayNote) const
{
   if (ImGui::TreeNode("Timer"))
   {
      ImGui::SliderScalar("Counter", ImGuiDataType_U32, &soundTimer.counter, &kZero, &soundTimer.period);
      ImGui::SliderScalar("Period", ImGuiDataType_U32, &soundTimer.period, &kZero, &maxPeriod);

      if (displayNote)
      {
         std::string pitch = getPitch(soundTimer.period);
         ImGui::Text("Pitch: %s", pitch.c_str());
      }

      ImGui::TreePop();
   }
}

void UI::renderDutyUnit(GBC::DutyUnit& dutyUnit) const
{
   if (ImGui::TreeNode("Duty unit"))
   {
      ImGui::Checkbox("High", &dutyUnit.high);

      uint8_t maxCounter = 7;
      ImGui::SliderScalar("Counter", ImGuiDataType_U8, &dutyUnit.counter, &kZero, &maxCounter);

      static const auto kHistogramGetter = [](void* data, int counter)
      {
         GBC::DutyUnit* dutyUnit = reinterpret_cast<GBC::DutyUnit*>(data);
         return GBC::DutyUnit::kDutyMasks[dutyUnit->index][counter] ? 1.0f : 0.0f;
      };

      ImGui::PlotHistogram("Duty", kHistogramGetter, &dutyUnit, maxCounter + 1);

      uint8_t maxIndex = 3;
      ImGui::SliderScalar("Index", ImGuiDataType_U8, &dutyUnit.index, &kZero, &maxIndex);

      ImGui::TreePop();
   }
}

void UI::renderLengthUnit(GBC::LengthUnit& lengthUnit) const
{
   if (ImGui::TreeNode("Length unit"))
   {
      ImGui::Checkbox("Enabled", &lengthUnit.enabled);
      ImGui::SliderScalar("Counter", ImGuiDataType_U16, &lengthUnit.counter, &kZero, &lengthUnit.maxCounter);
      ImGui::SliderScalar("Counter load", ImGuiDataType_U8, &lengthUnit.counterLoad, &kZero, &lengthUnit.maxCounter);

      ImGui::TreePop();
   }
}

void UI::renderEnvelopeUnit(GBC::EnvelopeUnit& envelopeUnit) const
{
   if (ImGui::TreeNode("Envelope unit"))
   {
      ImGui::Checkbox("DAC powered", &envelopeUnit.dacPowered);
      ImGui::Checkbox("Enabled", &envelopeUnit.enabled);
      ImGui::Checkbox("Add mode", &envelopeUnit.addMode);

      uint8_t maxCounter = 8;
      ImGui::SliderScalar("Counter", ImGuiDataType_U8, &envelopeUnit.counter, &kZero, &maxCounter);

      uint8_t maxPeriod = 0x07;
      ImGui::SliderScalar("Period", ImGuiDataType_U8, &envelopeUnit.period, &kZero, &maxPeriod);

      uint8_t maxVolume = 0x0F;
      ImGui::SliderScalar("Volume", ImGuiDataType_U8, &envelopeUnit.volume, &kZero, &maxVolume);
      ImGui::SliderScalar("Volume load", ImGuiDataType_U8, &envelopeUnit.volumeLoad, &kZero, &maxVolume);

      ImGui::TreePop();
   }
}

void UI::renderSweepUnit(GBC::SweepUnit& sweepUnit) const
{
   if (ImGui::TreeNode("Sweep unit"))
   {
      ImGui::Checkbox("Enabled", &sweepUnit.enabled);
      ImGui::Checkbox("Negate", &sweepUnit.negate);

      uint16_t maxFrequency = 0x07FF;
      ImGui::SliderScalar("Shadow frequency", ImGuiDataType_U16, &sweepUnit.shadowFrequency, &kZero, &maxFrequency);

      uint8_t maxCounter = 8;
      ImGui::SliderScalar("Counter", ImGuiDataType_U8, &sweepUnit.counter, &kZero, &maxCounter);

      uint8_t maxPeriod = 0x07;
      ImGui::SliderScalar("Period", ImGuiDataType_U8, &sweepUnit.period, &kZero, &maxPeriod);

      uint8_t maxShift = 0x07;
      ImGui::SliderScalar("Shift", ImGuiDataType_U8, &sweepUnit.shift, &kZero, &maxShift);

      ImGui::TreePop();
   }
}

void UI::renderWaveUnit(GBC::WaveUnit& waveUnit) const
{
   if (ImGui::TreeNode("Wave unit"))
   {
      ImGui::Checkbox("DAC powered", &waveUnit.dacPowered);

      uint8_t maxPosition = 31;
      ImGui::SliderScalar("Position", ImGuiDataType_U8, &waveUnit.position, &kZero, &maxPosition);

      uint8_t maxVolumeCode = 0x03;
      ImGui::SliderScalar("Volume code", ImGuiDataType_U8, &waveUnit.volumeCode, &kZero, &maxVolumeCode);

      static const auto kHistogramGetter = [](void* data, int position)
      {
         GBC::WaveUnit* waveUnit = reinterpret_cast<GBC::WaveUnit*>(data);

         uint8_t sampleIndex = position / 2;
         uint8_t value = waveUnit->waveTable[sampleIndex];
         uint8_t sample = position % 2 == 0 ? ((value & 0xF0) >> 4) : (value & 0x0F);

         return static_cast<float>(sample);
      };

      ImGui::PlotHistogram("Wave table", kHistogramGetter, &waveUnit, static_cast<int>(waveUnit.waveTable.size() * 2), 0, nullptr, 0.0f, 15.0f, ImVec2(0, 100));

      ImGui::TreePop();
   }
}

void UI::renderLFSRUnit(GBC::LFSRUnit& lfsrUnit) const
{
   if (ImGui::TreeNode("LFSR unit"))
   {
      ImGui::Checkbox("Width mode", &lfsrUnit.widthMode);

      uint8_t maxClockShift = 0x0F;
      ImGui::SliderScalar("Clock shift", ImGuiDataType_U8, &lfsrUnit.clockShift, &kZero, &maxClockShift);

      uint8_t maxDivisorCode = 0x07;
      ImGui::SliderScalar("Divisor code", ImGuiDataType_U8, &lfsrUnit.divisorCode, &kZero, &maxDivisorCode);

      static const auto kHistogramGetter = [](void* data, int position)
      {
         GBC::LFSRUnit* lfsrUnit = reinterpret_cast<GBC::LFSRUnit*>(data);

         bool high = (lfsrUnit->lfsr & (0x01 << position)) == 0;
         return high ? 1.0f : 0.0f;
      };

      ImGui::PlotHistogram("Linear feedback shift register", kHistogramGetter, &lfsrUnit, 16, 0, nullptr, 0.0f, 1.0f);

      ImGui::TreePop();
   }
}
