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
        int32_t m_nextLabel;
        std::ostream& m_out;

    public:
        CodeGenerator(std::ostream& output);
        virtual ~CodeGenerator();

        bool TranslateFunctions(const std::vector<func>& funcList);

        // Translates the given statement recursively, and outputs 
        // the resulting assembly code into m_out.
        bool TranslateStatement(const stmt& stmt);

        // Translates the given expression recursively, and outputs 
        // the resulting assembly code into m_out.
        bool TranslateExpression(const expr& expr);

    private:
        virtual void stmt_assignment(const stmt& s) = 0;
        virtual void stmt_func_call(const stmt& s) = 0;
        virtual void stmt_branch(const stmt& s) = 0;
        virtual void stmt_creation(const stmt& s) = 0;
        virtual void stmt_destruction(const stmt& s) = 0;
        virtual void stmt_return(const stmt& s) = 0;

        virtual void expr_id(const expr& e) = 0;
        virtual void expr_id_offset(const expr& e) = 0;
        virtual void expr_literal(const expr& e) = 0;
        virtual void expr_arithmetic(const expr& e) = 0;
        virtual void expr_logical(const expr& e) = 0;
        virtual void expr_comparison(const expr& e) = 0;
        virtual void expr_func_call(const expr& e) = 0;
        virtual void expr_unary(const expr& e) = 0;

        // Builds a single assembly instruction as a string, based on the given operands.
        std::string BuildAsm(std::initializer_list<std::string>& operands);

        template<typename ...T>
        inline void Error(const T&... args)
        {
            std::cerr << "Code generation error: ";
            int expansion[sizeof...(T)] = {(std::cerr << args)...};
            std::cerr << std::endl;
        }

    protected:
        bool m_hasError;
        std::string m_elseLabel;
        std::string m_endLabel;
        std::string m_lastInstr;
        std::string m_ifLabel;

        inline std::string CreateLabel()
        {
            return ".L" + std::to_string(m_nextLabel++);
        }

        inline void WriteAsm(std::initializer_list<std::string>&& operands)
        {
            m_lastInstr = *operands.begin();
            m_out << BuildAsm(operands) << std::endl;
        }

        inline void WriteLabel(const std::string& label)
        {
            m_lastInstr = label;
            m_out << label << ':' << std::endl;
        }

        inline bool GetLastInstrIsBranch() const
        {
            return m_lastInstr == "BRZ" || m_lastInstr == "BRNZ";
        }
    };

    ////======== StackCodeGenerator ========////
    class StackCodeGenerator : public CodeGenerator
    {
    public:
        StackCodeGenerator(std::ostream& output);
        ~StackCodeGenerator();
    private:
        virtual void stmt_assignment(const stmt& s);
        virtual void stmt_func_call(const stmt& s);
        virtual void stmt_branch(const stmt& s);
        virtual void stmt_creation(const stmt& s);
        virtual void stmt_destruction(const stmt& s);
        virtual void stmt_return(const stmt& s);

        virtual void expr_id(const expr& e);
        virtual void expr_id_offset(const expr& e);
        virtual void expr_literal(const expr& e);
        virtual void expr_arithmetic(const expr& e);
        virtual void expr_logical(const expr& e);
        virtual void expr_comparison(const expr& e);
        virtual void expr_func_call(const expr& e);
        virtual void expr_unary(const expr& e);
    };

    ////======== RegisterCodeGenerator ========////
    class RegisterCodeGenerator : public CodeGenerator
    {
    public:
        RegisterCodeGenerator(std::ostream& output);
        ~RegisterCodeGenerator();

    private:
        virtual void stmt_assignment(const stmt& s);
        virtual void stmt_func_call(const stmt& s);
        virtual void stmt_branch(const stmt& s);
        virtual void stmt_creation(const stmt& s);
        virtual void stmt_destruction(const stmt& s);
        virtual void stmt_return(const stmt& s);

        virtual void expr_id(const expr& e);
        virtual void expr_id_offset(const expr& e);
        virtual void expr_literal(const expr& e);
        virtual void expr_arithmetic(const expr& e);
        virtual void expr_logical(const expr& e);
        virtual void expr_comparison(const expr& e);
        virtual void expr_func_call(const expr& e);
        virtual void expr_unary(const expr& e);
    };

}

#endif // INC_CODE_GENERATOR_HPP