#ifndef INC_COMMON_HPP
#define INC_COMMON_HPP

#include <cstdint>

namespace Assembly
{
    using Word = uint32_t;
    using Instruction = uint32_t;
    using Address = uint32_t;
    using Register = uint8_t;

    enum VMMode
    {
        VM_MODE_REGISTER = 0,
        VM_MODE_STACK    = 1
    };
}

#endif // INC_COMMON_HPP