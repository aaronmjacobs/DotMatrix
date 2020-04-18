#pragma once

#include <type_traits>

namespace Enum
{
   template<typename E>
   constexpr auto cast(E e) -> typename std::underlying_type<E>::type
   {
      return static_cast<typename std::underlying_type<E>::type>(e);
   }
}
