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

#ifndef INC_COMPILER_HPP
#define INC_COMPILER_HPP

#include <string>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>

#include "parser.hpp"
#include "lexer.hpp"

namespace Compiler
{
    class RackCompiler
    {
    private:
        std::vector<func> m_funcList;

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

        void AddFunction(DataType dataType, std::string&& id, std::vector<stmt>&& statements);
    };
}

#endif // INC_COMPILER_HPP