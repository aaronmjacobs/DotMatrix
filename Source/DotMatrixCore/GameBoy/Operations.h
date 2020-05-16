#pragma once

#include "DotMatrixCore/GameBoy/CPU.h"

#include <array>

namespace DotMatrix
{

extern const std::array<Operation, 256> kOperations;
extern const std::array<Operation, 256> kCBOperations;

} // namespace DotMatrix
