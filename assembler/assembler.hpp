#ifndef INC_ASSEMBLER_HPP
#define INC_ASSEMBLER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "label.hpp"

namespace Assembly 
{
    constexpr uint32_t DEFAULT_MODE     = VM_MODE_REGISTER;
    constexpr uint32_t DEFAULT_HEAP     = 4096;             // 4 MiB.
    constexpr uint32_t DEFAULT_HEAP_MAX = 67108864;         // 64 MiB.

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
        size_t m_lineNbr;
        Address m_instrNbr;
        std::stringstream workingText;
        BinaryHeader m_binHeader;
        LabelDictionary m_labelDict;


        std::string RemoveWhitespace(const std::string& str) const;

        void ExecAssemblerDirective(const std::string& directive, std::string args[3]);

        void FirstPassReadLine(std::string& line);
        Instruction AssembleLine(std::string& line);

    public:
        Assembler();

        // Attempts to assemble the textInput stream of assembly into its corresponding binary format.
        // Returns the number of instructions in the generated binary file.
        size_t Assemble(std::ifstream& textInput, const std::string& outPath, bool verbose=false);
    };

}

#endif // INC_ASSEMBLER_HPP