#ifndef INC_COMMON_HPP
#define INC_COMMON_HPP

#include <cstdint>

namespace Assembly
{
    using Instruction = uint32_t;
    using Address = uint32_t;

    constexpr uint32_t VM_MODE_REGISTER = 0;
    constexpr uint32_t VM_MODE_STACK = 1;
}

#endif // INC_COMMON_HPP