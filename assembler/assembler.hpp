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

#ifndef INC_ASSEMBLER_HPP
#define INC_ASSEMBLER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

#include "label.hpp"
#include "encoder.hpp"

namespace Assembly 
{
    constexpr uint32_t DEFAULT_MODE     = VM_MODE_REGISTER;
    constexpr uint32_t DEFAULT_HEAP     = 4096;             // 4 MiB.
    constexpr uint32_t DEFAULT_HEAP_MAX = 67108864;         // 64 MiB.

    using AssemblerFlags = unsigned int;

    constexpr AssemblerFlags FLAG_SHOW_FIRST_PASS = 0x1;
    constexpr AssemblerFlags FLAG_SHOW_TRANSLATION = 0x2;
    constexpr AssemblerFlags FLAG_SUPPRESS_UNUSED_LABELS = 0x4;
    constexpr AssemblerFlags FLAG_SUPPRESS_ALL_ERRORS = 0x8;

    constexpr AssemblerFlags FLAG_VERBOSE = FLAG_SHOW_TRANSLATION;

    // Forward declare
    extern BinaryInstruction TranslateInstruction(const std::string& opcode, uint64_t* args);

    struct BinaryHeader
    {
        uint32_t mode, heap, heap_max, dataStart;
        BinaryHeader() : 
            mode(DEFAULT_MODE), 
            heap(DEFAULT_HEAP), 
            heap_max(DEFAULT_HEAP_MAX),
            dataStart(UINT32_MAX)
        {}
    };

    class Assembler
    {
    private:
        bool m_hasError;
        size_t m_lastError;
        size_t m_lineNbr;
        Address m_instrAddr;            // Instruction addresses are all in bytes.
        AssemblerFlags m_flags;
        std::stringstream workingText;
        BinaryHeader m_binHeader;
        LabelDictionary m_labelDict;
        InstructionEncoder m_encoder;

    public:
        Assembler();

        // Attempts to assemble the textInput stream of assembly into its corresponding binary format.
        // Returns the number of bytes that was successfully generated.
        // Returns 0 on assembly error.
        size_t Assemble(std::istream& textInput, std::iostream& binaryOutput);

        // If no flags are set, the assembler will show all errors and warnings by default.
        inline void SetFlags(AssemblerFlags flags) { m_flags = flags; }

    private:
        std::string ToLowerCase(const std::string& str) const;
        std::string RemoveWhitespace(const std::string& str) const;
        void PrintMemory(void* address, size_t byteCount) const;
        BinaryInstruction TranslateInstruction(const std::string& opcode, uint64_t* args);
        uint64_t EvaluateArgument(const std::string& arg);
        void ExecAssemblerDirective(const std::string& directive, const std::string* args);
        void FirstPassReadLine(std::string& line);
        void AssembleLine(std::string& line, std::iostream& binaryOutput);

    };

}

#endif // INC_ASSEMBLER_HPP