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
#include <algorithm>
#include "types.hpp"

namespace Compiler
{
    struct Instruction
    {
        std::vector<std::string> operands;
        int32_t jumpTo;     // > -1 if should jump to the given instruction. Index into instructions within the function.
        int32_t jumpAfter;  // > -1 if should jump to the instruction directly after the given instruction.
        std::string label;                            // "" if there is no label.
        bool done;

        Instruction() : 
            Instruction(std::vector<std::string>())
        {
        }

        Instruction(std::vector<std::string>&& operands) : 
            operands(std::move(operands)),
            jumpTo(-1),
            jumpAfter(-1),
            label(""),
            done(false)
        {
        }
    };

    ////======== CodeGenerator ========////

    // This class serves as the base class for RegisterCodeGenerator and 
    // StackCodeGenerator, which generate assembly code based on statements and expressions.
    class CodeGenerator
    {
    private:
        std::stringstream m_ss;
        int64_t m_nextLabel;
        std::map<std::string, std::vector<Instruction>> m_instr; // Contains a map of function ids and their instructions.
        std::string m_currFunc;

    public:
        CodeGenerator();
        virtual ~CodeGenerator();

        bool TranslateFunctions(const std::vector<func>& funcList);

        // Translates the given statement recursively, and outputs 
        // the resulting assembly code into m_out.
        bool TranslateStatement(const stmt& stmt);

        // Translates the given expression recursively, and outputs 
        // the resulting assembly code into m_out.
        bool TranslateExpression(const expr& expr);

        // Creates, and resolves labels, and flushes all instructions to the given output stream.
        void Flush(std::ostream& output);

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
        std::string BuildAsm(const std::vector<std::string>& operands);

        template<typename ...T>
        inline void Error(const T&... args)
        {
            std::cerr << "Code generation error: ";
            int expansion[sizeof...(T)] = {(std::cerr << args)...};
            std::cerr << std::endl;
        }

    protected:
        bool m_hasError;
        int32_t m_lastInstr; // Index of the last instruction added to the current function.

        inline std::string CreateLabel()
        {
            return "_L" + std::to_string(m_nextLabel++);
        }

        inline int32_t AddInstruction(std::vector<std::string>&& operands)
        {
            auto& instructions = m_instr.at(m_currFunc);
            instructions.push_back(std::move(operands));
            m_lastInstr = static_cast<int32_t>(instructions.size() - 1);
            return m_lastInstr;
        }

        inline Instruction* GetFuncInstr(int32_t index)
        {
            auto& instructions = m_instr.at(m_currFunc);
            if (index < 0 || index >= instructions.size())
                return nullptr;

            return &instructions[index];
        }
    };

    ////======== StackCodeGenerator ========////
    class StackCodeGenerator : public CodeGenerator
    {
    public:
        StackCodeGenerator();
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
        RegisterCodeGenerator();
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