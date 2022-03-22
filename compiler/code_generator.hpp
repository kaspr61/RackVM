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

#ifndef INC_CODE_GENERATOR_HPP
#define INC_CODE_GENERATOR_HPP

#include <ostream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "types.hpp"

namespace Compiler
{
    ////======== CodeGenerator ========////

    // This class serves as the base class for RegisterCodeGenerator and 
    // StackCodeGenerator, which generate assembly code based on statements and expressions.
    class CodeGenerator
    {
    private:
        std::stringstream m_ss;

    public:
        CodeGenerator();
        virtual ~CodeGenerator();

        bool TranslateFunctions(const std::vector<func>& funcList, std::ostream& output);

        // Translates the given statement recursively, and outputs 
        // the resulting assembly code into 'output'.
        bool TranslateStatement(const stmt& stmt, std::ostream& output);

        // Translates the given expression recursively, and outputs 
        // the resulting assembly code into 'output'.
        bool TranslateExpression(const expr& expr, std::ostream& output);

    private:
        virtual std::string stmt_assignment(const stmt& s, std::ostream& output) = 0;
        virtual std::string stmt_initialization(const stmt& s, std::ostream& output) = 0;
        virtual std::string stmt_func_call(const stmt& s, std::ostream& output) = 0;
        virtual std::string stmt_branch(const stmt& s, std::ostream& output) = 0;
        virtual std::string stmt_creation(const stmt& s, std::ostream& output) = 0;
        virtual std::string stmt_destruction(const stmt& s, std::ostream& output) = 0;

        virtual std::string expr_id(const expr& e, std::ostream& output) = 0;
        virtual std::string expr_literal(const expr& e, std::ostream& output) = 0;
        virtual std::string expr_arithmetic(const expr& e, std::ostream& output) = 0;
        virtual std::string expr_comparison(const expr& e, std::ostream& output) = 0;
        virtual std::string expr_func_call(const expr& e, std::ostream& output) = 0;

        template<typename ...T>
        inline void Error(const T&... args)
        {
            std::cerr << "Code generation error: ";
            int expansion[sizeof...(T)] = {(std::cerr << args)...};
            std::cerr << std::endl;
        }

    protected:
        bool m_hasError;

        // Builds a single assembly instruction as a string, based on the given operands.
        std::string BuildAsm(std::initializer_list<std::string>&& operands);
    };

    ////======== StackCodeGenerator ========////
    class StackCodeGenerator : public CodeGenerator
    {
    private:
        virtual std::string stmt_assignment(const stmt& s, std::ostream& output);
        virtual std::string stmt_initialization(const stmt& s, std::ostream& output);
        virtual std::string stmt_func_call(const stmt& s, std::ostream& output);
        virtual std::string stmt_branch(const stmt& s, std::ostream& output);
        virtual std::string stmt_creation(const stmt& s, std::ostream& output);
        virtual std::string stmt_destruction(const stmt& s, std::ostream& output);

        virtual std::string expr_id(const expr& e, std::ostream& output);
        virtual std::string expr_literal(const expr& e, std::ostream& output);
        virtual std::string expr_arithmetic(const expr& e, std::ostream& output);
        virtual std::string expr_comparison(const expr& e, std::ostream& output);
        virtual std::string expr_func_call(const expr& e, std::ostream& output);
    };

    ////======== RegisterCodeGenerator ========////
    class RegisterCodeGenerator : public CodeGenerator
    {
    private:
        virtual std::string stmt_assignment(const stmt& s, std::ostream& output);
        virtual std::string stmt_initialization(const stmt& s, std::ostream& output);
        virtual std::string stmt_func_call(const stmt& s, std::ostream& output);
        virtual std::string stmt_branch(const stmt& s, std::ostream& output);
        virtual std::string stmt_creation(const stmt& s, std::ostream& output);
        virtual std::string stmt_destruction(const stmt& s, std::ostream& output);

        virtual std::string expr_id(const expr& e, std::ostream& output);
        virtual std::string expr_literal(const expr& e, std::ostream& output);
        virtual std::string expr_arithmetic(const expr& e, std::ostream& output);
        virtual std::string expr_comparison(const expr& e, std::ostream& output);
        virtual std::string expr_func_call(const expr& e, std::ostream& output);
    };

}

#endif // INC_CODE_GENERATOR_HPP