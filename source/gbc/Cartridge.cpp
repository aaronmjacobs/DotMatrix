#include "FancyAssert.h"
#include "Log.h"

#include "gbc/Cartridge.h"
#include "gbc/Memory.h"

#include <cstring>
#include <memory>

namespace GBC {

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
   ASSERT(data && numBytes > 0x014F);

   uint8_t type = data[0x0147];
   switch (type) {
      case kROMOnly:
         return UPtr<Cartridge>(new SimpleCartridge(std::move(data), numBytes));
      default:
         LOG_ERROR_MSG_BOX("Invalid cartridge type");
         return nullptr;
   }
}

Cartridge::Cartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes)
   : data(std::move(cartData)), numBytes(cartNumBytes), title({ 0 }) {
   ASSERT(data && numBytes > 0x014F);

   strncpy(title.data(), reinterpret_cast<char*>(&data[0x0134]), title.size());
}

} // namespace GBC
