#pragma once

namespace Math
{
   template<typename I, typename F>
   I round(F value)
   {
      return static_cast<I>(value + static_cast<F>(0.5));
   }
}
