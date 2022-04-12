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

#include <string>
#include <unordered_map>
#include <functional>
#include <cstring>
#include "common.hpp"

namespace Assembly 
{
    // Instructions are always little-endian.
    struct BinaryInstruction
    {
        uint8_t opcode;     // Opcode is always 1 byte.
        Word instr[3];      // Operands can be 12 bytes at most.

        BinaryInstruction()
            : opcode(0), instr{0}
        {
        }

        BinaryInstruction(const BinaryInstruction& other)
        {
            opcode = other.opcode;
            std::memcpy(&(instr[0]), &(other.instr[0]), 12 /*bytes*/);
        }

        BinaryInstruction(BinaryInstruction&& other)
        {
            opcode = std::move(other.opcode);
            std::memmove(&(instr[0]), &(other.instr[0]), 12 /*bytes*/);
        }

        // For instructions without arguments.
        // Final binary size: 1 byte.
        BinaryInstruction(uint8_t opcode)
            : BinaryInstruction()
        {
            this->opcode = opcode;
        }

        // For instructions with argument: Ra
        // Final binary size: 2 bytes.
        BinaryInstruction(uint8_t opcode, Register regA)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr[0] = regA;
        }

        // For instructions with arguments: Ra, Rb
        // Final binary size: 3 bytes.
        BinaryInstruction(uint8_t opcode, Register regA, Register regB)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr[0] = regA;
            instr[0] |= (regB << 8)  & 0x0000'FF00;
        }

        // For instructions with arguments: Ra, Rb, Rc
        // Final binary size: 4 bytes.
        explicit BinaryInstruction(uint8_t opcode, Register regA, Register regB, Register regC)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr[0] = regA;
            instr[0] |= (regB << 8)  & 0x0000'FF00;
            instr[0] |= (regC << 16)  & 0x00FF'0000;
        }

        // For instructions with arguments: Ra, 32-bit C
        // Final binary size: 6 bytes.
        BinaryInstruction(uint8_t opcode, Register regA, uint32_t C)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr[0] = regA;
            instr[0] |= (C << 8)  & 0xFFFF'FF00; // Write 3/4 bytes of C.
            instr[1] = (C & 0xFF00'0000) >> 24;  // Write the remaining byte of C.
        }

        // For instructions with arguments: Ra, 64-bit C.
        // Final binary size: 10 bytes.
        BinaryInstruction(uint8_t opcode, Register regA, uint64_t C)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr[0] = regA;
            instr[0] |= (C << 8)  & 0xFFFF'FF00;          // Write 3/8 bytes of C.
            instr[1] = (C & 0x00FF'FFFF'FF00'0000) >> 24; // Write the remaining 4/5 bytes of C.
            instr[2] = (C & 0xFF00'0000'0000'0000) >> 56; // Write the remaining byte of C.
        }

        // For instructions with arguments: C
        // Final binary size: 5 bytes.
        BinaryInstruction(uint8_t opcode, uint32_t C)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr[0] = C;
        }

        // For instructions with arguments: C
        // Final binary size: 9 bytes.
        BinaryInstruction(uint8_t opcode, uint64_t C)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr[0] = (C & 0x0000'0000'FFFF'FFFF);       // Write 4/8 bytes of C.
            instr[1] = (C & 0xFFFF'FFFF'0000'0000) >> 32; // Write the remaining 4 bytes of C.
        }

        // For instructions with arguments: (float) C
        // Final binary size: 5 bytes.
        BinaryInstruction(uint8_t opcode, float C)
            : BinaryInstruction()
        {
            union 
            {
                float fVal;
                uint32_t iVal;
            } reinterpret;
            reinterpret.fVal = C;

            this->opcode = opcode;
            instr[0] = reinterpret.iVal;
        }

        // For instructions with arguments: (double) C
        // Final binary size: 9 bytes.
        BinaryInstruction(uint8_t opcode, double C)
            : BinaryInstruction()
        {
            union 
            {
                double dVal;
                uint64_t iVal;
            } reinterpret;
            reinterpret.dVal = C;

            this->opcode = opcode;
            instr[0] = (reinterpret.iVal & 0x0000'0000'FFFF'FFFF);       // Write 4/8 bytes of C.
            instr[1] = (reinterpret.iVal & 0xFFFF'FFFF'0000'0000) >> 32; // Write the remaining 4 bytes of C.
        }

        // For instructions with arguments: Ra, Rb, (uint32_t) C
        // Final binary size: 7 bytes.
        BinaryInstruction(uint8_t opcode, Register regA, Register regB, uint32_t C)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr[0] = regA;
            instr[0] |= (regB << 8)  & 0x0000'FF00;
            instr[0] |= (C << 16)  & 0xFFFF'0000;
            instr[1] = (C >> 16) & 0x0000'FFFF;
        }

        // For instructions with arguments: Ra, Rb, (uint64_t) C
        // Final binary size: 11 bytes.
        BinaryInstruction(uint8_t opcode, Register regA, Register regB, uint64_t C)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr[0] = regA;
            instr[0] |= (regB << 8)  & 0x0000'FF00;
            instr[0] |= (C << 16) & 0xFFFF'0000; // Write 2/8 bytes of C.
            instr[1] = (C >> 16);                // Write 4/6 remaining bytes of C.
            instr[2] = (C >> 48) & 0x0000'FFFF;  // Write 2 remaining bytes of C.
        }

        // For instructions with arguments: Ra, Rb, (float) C
        // Final binary size: 7 bytes.
        explicit BinaryInstruction(uint8_t opcode, Register regA, Register regB, float C)
            : BinaryInstruction(opcode, regA, regB, *(uint32_t*)(&C))
        {
        }

        // For instructions with arguments: Ra, Rb, (double) C
        // Final binary size: 11 bytes.
        explicit BinaryInstruction(uint8_t opcode, Register regA, Register regB, double C)
            : BinaryInstruction(opcode, regA, regB, *(uint64_t*)(&C))
        {
        }

    };

    struct InstructionData
    {
        size_t byteSize;
        uint64_t argMax[3];

        InstructionData() : 
            byteSize(1)
        {
            this->argMax[0] = 0;
            this->argMax[1] = 0;
            this->argMax[2] = 0;
        }

        InstructionData(size_t byteSize, uint64_t arg1Max=0, uint64_t arg2Max=0, uint64_t arg3Max=0)         
        {
            this->byteSize  = byteSize;
            this->argMax[0] = arg1Max;
            this->argMax[1] = arg2Max;
            this->argMax[2] = arg3Max;
        }
    };

    class InstructionEncoder
    {
    private:
        typedef BinaryInstruction(*TransFunc)(uint64_t, uint64_t, uint64_t);
        using TransDictionary = std::unordered_map<std::string, TransFunc>;
        using InfoDictionary = std::unordered_map<std::string, InstructionData>;

        TransDictionary m_translate;
        InfoDictionary m_info;

    public:
        InstructionEncoder();

        // Loads the instruction set used by the encoder.
        void LoadInstructionSet(const VMMode mode);

        inline bool IsInstructionValid(const std::string& opcode)
        {
            return m_translate.find(opcode) != m_translate.cend();
        }

	    inline BinaryInstruction TranslateInstruction(const std::string& opcode, uint64_t* args)
        {
            return m_translate[opcode](args[0], args[1], args[2]);
        }

	    size_t GetInstructionByteSize(const std::string& opcode) const;
	    uint64_t GetInstructionMaxArgSize(const std::string& opcode, int argIdx) const;
	    size_t GetInstructionArgCount(const std::string& opcode) const;
    };
}
