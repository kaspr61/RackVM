#ifndef INC_COMPILER_HPP
#define INC_COMPILER_HPP

#include <string>
#include <cstring>
#include <fstream>
#include <map>

#include "parser.hpp"
#include "lexer.hpp"

namespace Compiler
{
    class RackCompiler
    {
    public:
        std::map<std::string, int> m_vars;
        int m_result;
        std::string m_file;
        bool m_traceParsing;    // Whether to generate parsing debug traces.

        Compiler::location m_location;

    public:
        RackCompiler();
        ~RackCompiler();

        int Parse(const std::string& file);
    };
}

#endif // INC_COMPILER_HPP