#include "FancyAssert.h"
#include "Log.h"

#include "gbc/Cartridge.h"
#include "gbc/Memory.h"

#include <cstring>
#include <memory>

namespace GBC {

namespace {

static const uint16_t kHeaderOffset = 0x0100;
static const uint16_t kHeaderSize = 0x0050;

Cartridge::Header parseHeader(const uint8_t* data, size_t numBytes) {
   STATIC_ASSERT(sizeof(Cartridge::Header) == kHeaderSize);
   ASSERT(numBytes >= kHeaderOffset + kHeaderSize);

   Cartridge::Header header;
   memcpy(&header, data + kHeaderOffset, kHeaderSize);

   return header;
}

} // namespace

class SimpleCartridge : public Cartridge {
public:
   void tick(uint64_t cycles);

   void load(Memory& memory);

private:
   friend class Cartridge;
   SimpleCartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes);
};

SimpleCartridge::SimpleCartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes)
   : Cartridge(std::move(cartData), cartNumBytes) {
}

void SimpleCartridge::tick(uint64_t cycles) {
}

void SimpleCartridge::load(Memory& memory) {
   ASSERT(data && numBytes > 0 && numBytes <= 0x8000);

   memcpy(memory.romb, data.get(), numBytes);
}

// static
UPtr<Cartridge> Cartridge::fromData(UPtr<uint8_t[]>&& data, size_t numBytes) {
   if (!data || numBytes < kHeaderOffset + kHeaderSize) {
      return nullptr;
   }

   Header header = parseHeader(data.get(), numBytes);
   switch (header.cartridgeType) {
      case kROMOnly:
         return UPtr<Cartridge>(new SimpleCartridge(std::move(data), numBytes));
      default:
         LOG_ERROR_MSG_BOX("Invalid cartridge type");
         return nullptr;
   }
}

Cartridge::Cartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes)
   : data(std::move(cartData)), numBytes(cartNumBytes), header(parseHeader(data.get(), numBytes)), title({}) {
   ASSERT(data);

   // Copy title into separate array to ensure there is a null terminator
   memcpy(title.data(), header.title.data(), header.title.size());
}

} // namespace GBC
