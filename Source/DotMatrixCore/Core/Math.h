#pragma once

#include <cstdint>
#include <cstring>

namespace DotMatrix
{

namespace Math
{
   template<typename I, typename F>
   I round(F value)
   {
      return static_cast<I>(value + static_cast<F>(0.5));
   }

   inline int8_t reinterpretAsSigned(uint8_t value)
   {
      int8_t signedVal;
      std::memcpy(&signedVal, &value, sizeof(signedVal));
      return signedVal;
   }
}

} // namespace DotMatrix
