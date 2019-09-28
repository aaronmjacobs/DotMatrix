#pragma once

#include "GBC/CPU.h"

#include <array>

namespace GBC
{

extern const std::array<Operation, 256> kOperations;
extern const std::array<Operation, 256> kCBOperations;

} // namespace GBC
