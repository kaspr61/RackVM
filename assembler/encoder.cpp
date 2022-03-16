/*
BSD 2-Clause License

Copyright (c) 2022, Kasper Skott

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "encoder.hpp"

namespace Assembly
{
    //---- General purpose instruction macros ----------------------------------------------------//

    #define DECL_INSTR0(op, mn) BinaryInstruction _##mn(const uint64_t, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op); }

    #define DECL_INSTR1(op, mn, a) BinaryInstruction _##mn(const uint64_t Ra, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op, (a)Ra); }

    #define DECL_INSTR2(op, mn, a, b) BinaryInstruction _##mn(const uint64_t Ra, const uint64_t Rb, const uint64_t)\
    { return BinaryInstruction(op, (a)Ra, (b)Rb); }

    #define DECL_INSTR3(op, mn, a, b, c) BinaryInstruction _##mn(const uint64_t Ra, const uint64_t Rb, const uint64_t Rc)\
    { return BinaryInstruction(op, (a)Ra, (b)Rb, (c)Rc); }

    #define LOAD_INSTR0(mn, fn, wordSize) {m_translate[mn] = _##fn; m_info[mn] = InstructionData(wordSize);}
    #define LOAD_INSTR(mn, fn, wordSize, ...) {m_translate[mn] = _##fn; m_info[mn] = InstructionData(wordSize, __VA_ARGS__);}
 
    //---- Register instruction macros -----------------------------------------------------------//

    #define DECL_REG_INSTR0(op, mn) BinaryInstruction R_##mn(const uint64_t, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op); }

    #define DECL_REG_INSTR1(op, mn, a) BinaryInstruction R_##mn(const uint64_t Ra, const uint64_t, const uint64_t)\
    { return BinaryInstruction(op, (a)Ra); }

    #define DECL_REG_INSTR2(op, mn, a, b) BinaryInstruction R_##mn(const uint64_t Ra, const uint64_t Rb, const uint64_t)\
    { return BinaryInstruction(op, (a)Ra, (b)Rb); }

    #define DECL_REG_INSTR3(op, mn, a, b, c) BinaryInstruction R_##mn(const uint64_t Ra, const uint64_t Rb, const uint64_t Rc)\
    { return BinaryInstruction(op, (a)Ra, (b)Rb, (c)Rc); }

    #define LOAD_REG_INSTR0(mn, fn, wordSize) {m_translate[mn] = R_##fn; m_info[mn] = InstructionData(wordSize);}
    #define LOAD_REG_INSTR(mn, fn, wordSize, ...) {m_translate[mn] = R_##fn; m_info[mn] = InstructionData(wordSize, __VA_ARGS__);}

    //---- Stack instruction macros --------------------------------------------------------------//

    #define LOAD_STACK_INSTR0(mn, fn, wordSize) {m_translate[mn] = S_##fn; m_info[mn] = InstructionData(wordSize);}
    #define LOAD_STACK_INSTR(mn, fn, wordSize, ...) {m_translate[mn] = S_##fn; m_info[mn] = InstructionData(wordSize, __VA_ARGS__);}

    //--------------------------------------------------------------------------------------------//

    //---- General-purpose instructions ----//

    DECL_INSTR0(0x00, NOP)
    DECL_INSTR0(0x01, EXIT)

    //---- Register-only instructions ----//

    DECL_REG_INSTR2(0x00, MOV,      Register,   Register)
    DECL_REG_INSTR2(0x00, LDI,      Register,   uint32_t)
    DECL_REG_INSTR2(0x00, LDI_64,   Register,   uint64_t)
    DECL_REG_INSTR3(0x00, ADD,      Register,   Register,   Register)
    DECL_REG_INSTR3(0x00, ADD_64,   Register,   Register,   Register)
    DECL_REG_INSTR3(0x00, ADDI,     Register,   Register,   uint32_t)
    DECL_REG_INSTR3(0x00, ADDI_64,  Register,   Register,   uint64_t)

    //---- Stack-only instructions ----//


    //----------------------------------//

    InstructionEncoder::InstructionEncoder()
    {
    }

    void InstructionEncoder::LoadInstructionSet(const VMMode mode)
    {
        m_translate.clear();

        LOAD_INSTR0("NOP", NOP, 1);
        LOAD_INSTR0("EXIT", EXIT, 1);

        if (mode == VM_MODE_REGISTER)
        {
            // Load the instructions and their word size, 
            // followed by the maximum value for each argument.
            LOAD_REG_INSTR("LDI",       LDI,        2,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR("LDI.64",    LDI_64,     3,  UINT8_MAX,  UINT64_MAX);
            LOAD_REG_INSTR("ADD",       ADD,        1,  UINT8_MAX,  UINT8_MAX,  UINT8_MAX);
            LOAD_REG_INSTR("ADD.64",    ADD_64,     1,  UINT8_MAX,  UINT8_MAX,  UINT8_MAX);
            LOAD_REG_INSTR("ADDI",      ADDI,       2,  UINT8_MAX,  UINT8_MAX,  UINT32_MAX);
            LOAD_REG_INSTR("ADDI.64",   ADDI_64,    3,  UINT8_MAX,  UINT8_MAX,  UINT64_MAX);

        }
        else if (mode == VM_MODE_STACK)
        {
            
        }
    }
}
