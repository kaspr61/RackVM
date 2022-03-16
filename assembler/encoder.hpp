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
#include "common.hpp"

namespace Assembly 
{
    union BinaryInstruction
    {
        uint8_t opcode;
        Word instr4;
        Word instr8[2];
        Word instr12[3];

        BinaryInstruction()
        {
            instr12[0] = 0;
            instr12[1] = 0;
            instr12[2] = 0;
        }

        // For instructions without arguments.
        BinaryInstruction(const uint8_t opcode)
            : BinaryInstruction()
        {
            this->opcode = opcode;
        }

        // For instructions with argument: Ra
        BinaryInstruction(const uint8_t opcode, const Register regA)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr4 |= (regA << 8) & 0x0000'FF00;
        }

        // For instructions with arguments: Ra, Rb
        BinaryInstruction(const uint8_t opcode, const Register regA, const Register regB)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr4 |= (regA << 8)  & 0x0000'FF00;
            instr4 |= (regB << 16) & 0x00FF'0000;
        }

        // For instructions with arguments: Ra, Rb, Rc
        BinaryInstruction(const uint8_t opcode, const Register regA, const Register regB, const Register regC)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr4 |= (regA << 8)  & 0x0000'FF00;
            instr4 |= (regB << 16) & 0x00FF'0000;
            instr4 |= (regC << 24) & 0xFF00'0000;
        }

        // For instructions with arguments: Ra, 32-bit C
        BinaryInstruction(const uint8_t opcode, const Register regA, const uint32_t C)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr8[0] |= (regA << 8)  & 0x0000'FF00;
            instr8[1] = C;
        }

        // For instructions with arguments: Ra, 64-bit C
        BinaryInstruction(const uint8_t opcode, const Register regA, const uint64_t C)
            : BinaryInstruction()
        {
            this->opcode = opcode;
            instr12[0] |= (regA << 8)  & 0x0000'FF00;
            
            // Use memcpy to write over multiple elements.
            ::memcpy(&(instr12[1]), (const void*)C, sizeof(uint64_t)); 
        }
    };

    struct InstructionData
    {
        size_t wordSize;
        size_t argMax[3];

        InstructionData() : 
            wordSize(1)
        {
            this->argMax[0] = 0;
            this->argMax[1] = 0;
            this->argMax[2] = 0;
        }

        InstructionData(size_t wordSize, size_t arg1Max=0, size_t arg2Max=0, size_t arg3Max=0)         
        {
            this->wordSize  = wordSize;
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

	    inline size_t GetInstructionWordSize(const std::string& opcode)
        {
            return m_info[opcode].wordSize;
        }

	    inline size_t GetInstructionMaxArgSize(const std::string& opcode, int argIdx)
        {
            if (argIdx > 3)
                return 0;

            return m_info[opcode].argMax[argIdx];
        }

    };
}
