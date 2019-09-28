#include "Core/Enum.h"

#include "GBC/Cartridge.h"

#include "UI/UI.h"

#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>

namespace
{

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

std::size_t getRAMSizeBytes(GBC::Cartridge::RAMSize ramSize)
{
   switch (ramSize)
   {
   case GBC::Cartridge::RAMSize::None:
      return 0;
   case GBC::Cartridge::RAMSize::Size2KBytes:
      return 2048;
   case GBC::Cartridge::RAMSize::Size8KBytes:
      return 8192;
   case GBC::Cartridge::RAMSize::Size32KBytes:
      return 32768;
   case GBC::Cartridge::RAMSize::Size128KBytes:
      return 131072;
   case GBC::Cartridge::RAMSize::Size64KBytes:
      return 65536;
   default:
      return 0;
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

const char* getMbc3BankModeText(GBC::MBC3::BankRegisterMode mode)
{
   switch (mode)
   {
   case GBC::MBC3::BankRegisterMode::BankZero:
      return "RAM bank 0";
   case GBC::MBC3::BankRegisterMode::BankOne:
      return "RAM bank 1";
   case GBC::MBC3::BankRegisterMode::BankTwo:
      return "RAM bank 2";
   case GBC::MBC3::BankRegisterMode::BankThree:
      return "RAM bank 3";
   case GBC::MBC3::BankRegisterMode::RTCSeconds:
      return "RTC seconds";
   case GBC::MBC3::BankRegisterMode::RTCMinutes:
      return "RTC minutes";
   case GBC::MBC3::BankRegisterMode::RTCHours:
      return "RTC hours";
   case GBC::MBC3::BankRegisterMode::RTCDaysLow:
      return "RTC days (low)";
   case GBC::MBC3::BankRegisterMode::RTCDaysHigh:
      return "RTC days (high)";
   default:
      return "Unknown";
   }
}

} // namespace

void UI::renderCartridgeWindow(GBC::Cartridge* cart) const
{
   ImGui::Begin("Cartridge");

   if (cart == nullptr)
   {
      ImGui::Text("Please insert a cartridge");
      ImGui::End();
      return;
   }

   const GBC::Cartridge::Header& header = cart->header;

   GBC::MemoryBankController* mbc = cart->controller.get();
   ASSERT(mbc);

   if (ImGui::BeginTabBar("CartridgeTabBar"))
   {
      if (ImGui::BeginTabItem("Header"))
      {
         ImGui::Columns(2, "header");
         ImGui::Separator();

         ImGui::Text("Title: %s", cart->title());
         ImGui::NextColumn();

         ImGui::Text("Type: %s", GBC::Cartridge::getTypeName(header.type));
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

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Memory Bank Controller"))
      {
         std::size_t maxRomBank = std::max(cart->cartData.size() / 16384, static_cast<std::size_t>(1)) - 1;
         std::size_t ramSize = getRAMSizeBytes(header.ramSize);
         std::size_t maxRamBank = std::max(ramSize / 8192, static_cast<std::size_t>(1)) - 1;

         if (GBC::MBCNull* mbcNull = dynamic_cast<GBC::MBCNull*>(mbc))
         {
            ImGui::Text("No memory bank controller");
         }
         else if (GBC::MBC1* mbc1 = dynamic_cast<GBC::MBC1*>(mbc))
         {
            ImGui::Checkbox("RAM enabled", &mbc1->ramEnabled);
            ImGui::Text("Banking mode: %s", mbc1->bankingMode == GBC::MBC1::BankingMode::ROM ? "ROM" : "RAM");
            ImGui::SliderScalar("ROM bank number", ImGuiDataType_U8, &mbc1->romBankNumber, &kZero, &maxRomBank);
            ImGui::SliderScalar("RAM bank number", ImGuiDataType_U8, &mbc1->ramBankNumber, &kZero, &maxRamBank);
         }
         else if (GBC::MBC2* mbc2 = dynamic_cast<GBC::MBC2*>(mbc))
         {
            ImGui::Checkbox("RAM enabled", &mbc2->ramEnabled);
            ImGui::SliderScalar("ROM bank number", ImGuiDataType_U8, &mbc2->romBankNumber, &kZero, &maxRomBank);
         }
         else if (GBC::MBC3* mbc3 = dynamic_cast<GBC::MBC3*>(mbc))
         {
            static const char* kRtcId = "rtc";
            static const char* kLatchedRtcId = "rtc_latched";

            ImGui::Checkbox("RAM / RTC enabled", &mbc3->ramRTCEnabled);
            ImGui::Checkbox("RTC latched", &mbc3->rtcLatched);
            ImGui::InputScalar("Latch data", ImGuiDataType_U8, &mbc3->latchData);
            ImGui::Text("Banking mode: %s", getMbc3BankModeText(mbc3->bankRegisterMode));
            ImGui::SliderScalar("ROM bank number", ImGuiDataType_U8, &mbc3->romBankNumber, &kZero, &maxRomBank);
            ImGui::InputDouble("Time remainder", &mbc3->tickTime);

            ImGui::Columns(2, "rtc");
            ImGui::Separator();

            ImGui::Text("Real-time clock");
            ImGui::NextColumn();
            ImGui::Text("Latched real-time clock");
            ImGui::NextColumn();

            ImGui::Separator();

            ImGui::PushID(kRtcId);
            ImGui::InputScalar("Seconds", ImGuiDataType_U8, &mbc3->rtc.seconds);
            ImGui::PopID();
            ImGui::NextColumn();
            ImGui::PushID(kLatchedRtcId);
            ImGui::InputScalar("Seconds", ImGuiDataType_U8, &mbc3->rtcLatchedCopy.seconds);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::PushID(kRtcId);
            ImGui::InputScalar("Minutes", ImGuiDataType_U8, &mbc3->rtc.minutes);
            ImGui::PopID();
            ImGui::NextColumn();
            ImGui::PushID(kLatchedRtcId);
            ImGui::InputScalar("Minutes", ImGuiDataType_U8, &mbc3->rtcLatchedCopy.minutes);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::PushID(kRtcId);
            ImGui::InputScalar("Hours", ImGuiDataType_U8, &mbc3->rtc.hours);
            ImGui::PopID();
            ImGui::NextColumn();
            ImGui::PushID(kLatchedRtcId);
            ImGui::InputScalar("Hours", ImGuiDataType_U8, &mbc3->rtcLatchedCopy.hours);
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::PushID(kRtcId);
            uint16_t days = (mbc3->rtc.daysMsb << 8) | mbc3->rtc.daysLow;
            ImGui::InputScalar("Days", ImGuiDataType_U16, &days);
            mbc3->rtc.daysLow = days & 0x00FF;
            mbc3->rtc.daysHigh = (days >> 8) & 0x0001;
            ImGui::PopID();
            ImGui::NextColumn();
            ImGui::PushID(kLatchedRtcId);
            uint16_t daysLatched = (mbc3->rtcLatchedCopy.daysMsb << 8) | mbc3->rtcLatchedCopy.daysLow;
            ImGui::InputScalar("Days", ImGuiDataType_U16, &daysLatched);
            mbc3->rtcLatchedCopy.daysLow = daysLatched & 0x00FF;
            mbc3->rtcLatchedCopy.daysHigh = (daysLatched >> 8) & 0x0001;
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::PushID(kRtcId);
            bool carry = mbc3->rtc.daysCarry != 0;
            ImGui::Checkbox("Carry", &carry);
            mbc3->rtc.daysCarry = carry ? 1 : 0;
            ImGui::PopID();
            ImGui::NextColumn();
            ImGui::PushID(kLatchedRtcId);
            bool carryLatched = mbc3->rtcLatchedCopy.daysCarry != 0;
            ImGui::Checkbox("Carry", &carryLatched);
            mbc3->rtcLatchedCopy.daysCarry = carryLatched ? 1 : 0;
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::PushID(kRtcId);
            bool halt = mbc3->rtc.halt != 0;
            ImGui::Checkbox("Halt", &halt);
            mbc3->rtc.halt = halt ? 1 : 0;
            ImGui::PopID();
            ImGui::NextColumn();
            ImGui::PushID(kLatchedRtcId);
            bool haltLatched = mbc3->rtcLatchedCopy.halt != 0;
            ImGui::Checkbox("Halt", &haltLatched);
            mbc3->rtcLatchedCopy.halt = haltLatched ? 1 : 0;
            ImGui::PopID();
            ImGui::NextColumn();

            ImGui::Columns(1);
         }
         else if (GBC::MBC5* mbc5 = dynamic_cast<GBC::MBC5*>(mbc))
         {
            ImGui::Checkbox("RAM enabled", &mbc5->ramEnabled);
            ImGui::SliderScalar("ROM bank number", ImGuiDataType_U8, &mbc5->romBankNumber, &kZero, &maxRomBank);
            ImGui::SliderScalar("RAM bank number", ImGuiDataType_U8, &mbc5->ramBankNumber, &kZero, &maxRamBank);
         }
         else
         {
            ImGui::Text("Unsupported memory bank controller type");
         }

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("ROM"))
      {
         static MemoryEditor memoryEditor;
         memoryEditor.DrawContents(cart->cartData.data(), cart->cartData.size());

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("RAM"))
      {
         if (GBC::MBCNull* mbcNull = dynamic_cast<GBC::MBCNull*>(mbc))
         {
            ImGui::Text("No RAM");
         }
         else if (GBC::MBC1* mbc1 = dynamic_cast<GBC::MBC1*>(mbc))
         {
            static MemoryEditor memoryEditor;

            std::size_t ramSize = std::min(getRAMSizeBytes(header.ramSize), mbc1->ramBanks[0].size() * mbc1->ramBanks.size());
            memoryEditor.DrawContents(mbc1->ramBanks[0].data(), ramSize);
         }
         else if (GBC::MBC2* mbc2 = dynamic_cast<GBC::MBC2*>(mbc))
         {
            static MemoryEditor memoryEditor;

            std::size_t ramSize = std::min(getRAMSizeBytes(header.ramSize), mbc2->ram.size());
            memoryEditor.DrawContents(mbc2->ram.data(), ramSize);
         }
         else if (GBC::MBC3* mbc3 = dynamic_cast<GBC::MBC3*>(mbc))
         {
            static MemoryEditor memoryEditor;

            std::size_t ramSize = std::min(getRAMSizeBytes(header.ramSize), mbc3->ramBanks[0].size() * mbc3->ramBanks.size());
            memoryEditor.DrawContents(mbc3->ramBanks[0].data(), ramSize);
         }
         else if (GBC::MBC5* mbc5 = dynamic_cast<GBC::MBC5*>(mbc))
         {
            static MemoryEditor memoryEditor;

            std::size_t ramSize = std::min(getRAMSizeBytes(header.ramSize), mbc5->ramBanks[0].size() * mbc5->ramBanks.size());
            memoryEditor.DrawContents(mbc5->ramBanks[0].data(), ramSize);
         }
         else
         {
            ImGui::Text("Unsupported memory bank controller type");
         }

         ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
   }

   ImGui::End();
}
