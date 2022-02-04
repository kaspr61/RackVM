#ifndef INC_ASSEMBLER_HPP
#define INC_ASSEMBLER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

#include "label.hpp"

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
        Address m_instrNbr;
        AssemblerFlags m_flags;
        std::stringstream workingText;
        BinaryHeader m_binHeader;
        LabelDictionary m_labelDict;

        std::string RemoveWhitespace(const std::string& str) const;
        uint32_t EvaluateArgument(const std::string& arg);
        void ExecAssemblerDirective(const std::string& directive, std::string args[3]);
        void FirstPassReadLine(std::string& line);
        Instruction AssembleLine(std::string& line);

    public:
        Assembler();

        // Attempts to assemble the textInput stream of assembly into its corresponding binary format.
        // Returns the number of instructions in the generated binary file.
        size_t Assemble(std::ifstream& textInput, const std::string& outPath);

        // If no flags are set, the assembler will show all errors and warnings by default.
        inline void SetFlags(AssemblerFlags flags) { m_flags = flags; }
    };

}

#endif // INC_ASSEMBLER_HPP