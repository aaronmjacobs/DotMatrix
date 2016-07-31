#include "FancyAssert.h"

#include "gbc/Bootstrap.h"
#include "gbc/Memory.h"

namespace GBC {

Memory::Memory()
   : raw{}, bootstrap(kBootstrap) {
}

uint8_t& Memory::operator[](size_t index) {
   ASSERT(index < 0x10000);

   if (boot == 0 && index <= 0x00FF) {
      return bootstrap[index];
   }

   if (index >= 0xE000 && index < 0xFE00) {
      // Mirror of working ram
      index -= 0x2000;
   }

   return raw[index];
}

} // namespace GBC
