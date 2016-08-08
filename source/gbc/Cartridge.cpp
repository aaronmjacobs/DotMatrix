#include "FancyAssert.h"
#include "Log.h"

#include "gbc/Cartridge.h"
#include "gbc/Memory.h"

#if !defined(NDEBUG)
#  include "Debug.h"
#endif // !defined(NDEBUG)

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

bool performHeaderChecksum(const Cartridge::Header& header, const uint8_t* mem) {
   // Complement check, program will not run if incorrect
   // x=0:FOR i=0134h TO 014Ch:x=x-MEM[i]-1:NEXT

   uint8_t x = 0;
   for (uint16_t i = 0x0134; i <= 0x014C; ++i) {
      x = x - mem[i] - 1;
   }

   return x == header.headerChecksum;
}

bool performGlobalChecksum(const Cartridge::Header& header, const uint8_t* mem, size_t numBytes) {
   // Checksum (higher byte first) produced by adding all bytes of a cartridge except for two checksum bytes and taking
   // two lower bytes of the result. (GameBoy ignores this value.)

   uint16_t x = 0;
   for (size_t i = 0; i < numBytes; ++i) {
      x += mem[i];
   }

   x -= header.globalChecksum[0];
   x -= header.globalChecksum[1];

   uint8_t low = x & 0x00FF;
   uint8_t high = (x & 0xFF00) >> 8;

   return header.globalChecksum[0] == high && header.globalChecksum[1] == low;
}

} // namespace

class SimpleCartridge : public Cartridge {
public:
   const uint8_t* get(uint16_t address) const override;

   void set(uint16_t address, uint8_t val) override;

private:
   friend class Cartridge;
   SimpleCartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes);
};

SimpleCartridge::SimpleCartridge(UPtr<uint8_t[]>&& cartData, size_t cartNumBytes)
   : Cartridge(std::move(cartData), cartNumBytes) {
}

const uint8_t* SimpleCartridge::get(uint16_t address) const {
   ASSERT(address < numBytes);

   return &data[address];
}

void SimpleCartridge::set(uint16_t address, uint8_t val) {
   ASSERT(address < numBytes);

   // No writable memory
   LOG_WARNING("Trying to write to read-only cartridge at location " << hex(address) << ": " << hex(val));
}

// static
UPtr<Cartridge> Cartridge::fromData(UPtr<uint8_t[]>&& data, size_t numBytes) {
   if (!data || numBytes < kHeaderOffset + kHeaderSize) {
      LOG_ERROR("Cartridge provided insufficient data");
      return nullptr;
   }

   Header header = parseHeader(data.get(), numBytes);
   if (!performHeaderChecksum(header, data.get())) {
      LOG_ERROR("Cartridge data failed header checksum");
      return nullptr;
   }
   if (!performGlobalChecksum(header, data.get(), numBytes)) {
      LOG_ERROR("Cartridge data failed global checksum");
      return nullptr;
   }

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
