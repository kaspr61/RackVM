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
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>

#include "parser.hpp"
#include "lexer.hpp"
#include "code_generator.hpp"

namespace Compiler
{
    class RackCompiler
    {
    public:
        enum class CodeGenerationType
        {
            NONE,
            STACK,
            REGISTER
        };

    private:
        using ScopedVars = std::map<std::string, identifier>;

        RackParser* m_parser;
        CodeGenerationType m_codeGenType;
        std::vector<func> m_funcList;
        std::vector<ScopedVars> m_scopes;
        func m_currFunction;

    public:
        std::string m_file;
        bool m_traceParsing;    // Whether to generate parsing debug traces.

        Compiler::location m_location;

    public:
        RackCompiler() = delete;
        ~RackCompiler();

        RackCompiler(CodeGenerationType codeGenType);

        int Parse(const std::string& file);

        void  AddFunc(std::vector<stmt>&& statements);
        void  DeclFunc(DataType dataType, std::string&& id, std::vector<stmt>&& args);
        const identifier& DeclVar(DataType dataType, std::string&& varId, identifier_type idType);
        const identifier& UseVar(std::string&& varId) const;
        const func& UseFunc(const std::string& funcId, const std::list<expr>& args) const;
        std::string CheckReturnType(const stmt& retStmt) const;
        std::string CheckArrayCreation(stmt& st);
        
        inline void EnterScope() { m_scopes.emplace_back(); }
        inline void ExitScope()  { m_scopes.pop_back(); }

    private:
        bool MatchFunctionArgs(const func& fun, const std::list<expr>& args) const;

    };
}

#endif // INC_COMPILER_HPP