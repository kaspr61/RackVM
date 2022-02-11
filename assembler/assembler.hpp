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
        uint32_t mode, heap, heap_max;
        BinaryHeader() : 
            mode(DEFAULT_MODE), 
            heap(DEFAULT_HEAP), 
            heap_max(DEFAULT_HEAP_MAX)
        {}
    };

    class Assembler
    {
    private:
        bool m_hasError;
        size_t m_lastError;
        size_t m_lineNbr;
        Address m_instrAddr;            // Instruction addresses are all in word-space (4 bytes per word).
        AssemblerFlags m_flags;
        std::stringstream workingText;
        BinaryHeader m_binHeader;
        LabelDictionary m_labelDict;

        std::string ToLowerCase(const std::string& str) const;
        std::string RemoveWhitespace(const std::string& str) const;
        size_t GetInstructionSize(const std::string& opcode, uint64_t arg0) const;
        BinaryInstruction TranslateInstruction(const std::string& opcode, uint64_t* args) const;
        uint32_t EvaluateArgument(const std::string& arg);
        void ExecAssemblerDirective(const std::string& directive, const std::string* args);
        void FirstPassReadLine(std::string& line);
        void AssembleLine(std::string& line, std::iostream& binaryOutput);

    public:
        Assembler();

        // Attempts to assemble the textInput stream of assembly into its corresponding binary format.
        // Returns the number of bytes that was successfully generated.
        // Returns 0 on assembly error.
        size_t Assemble(std::istream& textInput, std::iostream& binaryOutput);

        // If no flags are set, the assembler will show all errors and warnings by default.
        inline void SetFlags(AssemblerFlags flags) { m_flags = flags; }
    };

}

#endif // INC_ASSEMBLER_HPP