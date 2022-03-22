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

#include "code_generator.hpp"

#define STR(x) std::to_string(x)
#define MOV(x) std::move(x)
#define RETURN_ERROR() {m_hasError = true; return std::string("error: ").append(__FUNCTION__);}

namespace Compiler
{
    //---- CodeGenerator Implementation ----//

    CodeGenerator::CodeGenerator() :
        m_ss(), m_hasError(false)     
        {}

    CodeGenerator::~CodeGenerator() {}

    bool CodeGenerator::TranslateFunctions(const std::vector<func>& funcList, std::ostream& output)
    {
        auto funcIt = funcList.begin();
        while (funcIt != funcList.end())
        {
            output << funcIt->id.id << ':' << std::endl;

            auto stmtIt = funcIt->statements.begin();
            while (stmtIt != funcIt->statements.end())
                TranslateStatement(*stmtIt++, output);

            ++funcIt;
        }

        return !m_hasError;
    }

    bool CodeGenerator::TranslateStatement(const stmt& s, std::ostream& output)
    {
        std::vector<stmt>::const_iterator substmt;

        switch (s.type)
        {
            case stmt_type::ASSIGNMENT:
            case stmt_type::ASSIGN_OFFSET: output << stmt_assignment(s, output) << std::endl;
                break;

            case stmt_type::INITIALIZATION: output << stmt_initialization(s, output) << std::endl;
                break;

            case stmt_type::FUNC_CALL: output << stmt_func_call(s, output) << std::endl;
                break;

            case stmt_type::BRANCH: output << stmt_branch(s, output) << std::endl;
                break;

            case stmt_type::BLOCK: 
                substmt = s.substmts.begin();
                while (substmt != s.substmts.end())
                    TranslateStatement(*substmt++, output);
                break;

            case stmt_type::CREATION: output << stmt_creation(s, output) << std::endl;
                break;

            case stmt_type::DESTRUCTION: output << stmt_destruction(s, output) << std::endl;
                break;

            default: break;
        }

        return !m_hasError;
    }

    bool CodeGenerator::TranslateExpression(const expr& e, std::ostream& output)
    {
        switch (e.type)
        {
            case expr_type::ID:
            case expr_type::ID_OFFSET: output << expr_id(e, output) << std::endl;
                break;

            case expr_type::NUMBER:
            case expr_type::STRING: output << expr_literal(e, output) << std::endl;
                break;

            case expr_type::ADD:
            case expr_type::SUB:
            case expr_type::MUL:
            case expr_type::DIV: output << expr_arithmetic(e, output) << std::endl;
                break;
        }

        return !m_hasError;
    }

    std::string CodeGenerator::BuildAsm(std::initializer_list<std::string>&& operands)
    {
        if (operands.size() < 1)
            return std::string("error: ").append(__FUNCTION__);

        m_ss.clear();
        m_ss.str("");
        
        // Format a line representing an assembly instruction.
        auto it = operands.begin();
        m_ss << "  " << std::setw(6) << std::left << *it++; // First operand (the mnemonic) has width 6
        
        while (it != operands.end())
        m_ss << "  " << std::setw(8) << std::left << *it++; // Other operands have width 8

        return m_ss.str();
    }

    //---- StackCodeGenerator Implementation ----//

    std::string StackCodeGenerator::stmt_assignment(const stmt& s, std::ostream& output)
    {
        TranslateExpression(s.expressions.front(), output);

        if (s.id.type == identifier_type::LOCAL_VAR)
            return BuildAsm({"LDL", STR(s.id.position)});
        else if (s.id.type == identifier_type::PARAM_VAR)
            return BuildAsm({"LDP", STR(s.id.position)});

        RETURN_ERROR();
    }

    std::string StackCodeGenerator::stmt_initialization(const stmt& s, std::ostream& output)
    {
        TranslateExpression(s.expressions.front(), output);

        if (s.id.type == identifier_type::LOCAL_VAR)
            return BuildAsm({"LDL", STR(s.id.position)});
        else if (s.id.type == identifier_type::PARAM_VAR)
            return BuildAsm({"LDP", STR(s.id.position)});

        RETURN_ERROR();
    }

    std::string StackCodeGenerator::stmt_func_call(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string StackCodeGenerator::stmt_branch(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string StackCodeGenerator::stmt_creation(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string StackCodeGenerator::stmt_destruction(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }


    std::string StackCodeGenerator::expr_id(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string StackCodeGenerator::expr_literal(const expr& e, std::ostream& output)
    {
        if (e.dataType == DataType::INT)
            return BuildAsm({"LDI", STR(e.intValue)});
        else if (e.dataType == DataType::LONG)
            return BuildAsm({"LDI.64", STR(e.longValue)});

        RETURN_ERROR();
    }

    std::string StackCodeGenerator::expr_arithmetic(const expr& e, std::ostream& output)
    {
        TranslateExpression(e.operands.front(), output);
        TranslateExpression(e.operands.back(), output);

        if (e.dataType == DataType::INT)
        {
            if      (e.type == expr_type::ADD) return BuildAsm({"ADD"});
            else if (e.type == expr_type::SUB) return BuildAsm({"SUB"});
            else if (e.type == expr_type::MUL) return BuildAsm({"MUL"});
            else if (e.type == expr_type::DIV) return BuildAsm({"DIV"});
        }
        else if (e.dataType == DataType::LONG)
        {
            if      (e.type == expr_type::ADD) return BuildAsm({"ADD.64"});
            else if (e.type == expr_type::SUB) return BuildAsm({"SUB.64"});
            else if (e.type == expr_type::MUL) return BuildAsm({"MUL.64"});
            else if (e.type == expr_type::DIV) return BuildAsm({"DIV.64"});
        }
        else if (e.dataType == DataType::FLOAT)
        {
            if      (e.type == expr_type::ADD) return BuildAsm({"ADD.F"});
            else if (e.type == expr_type::SUB) return BuildAsm({"SUB.F"});
            else if (e.type == expr_type::MUL) return BuildAsm({"MUL.F"});
            else if (e.type == expr_type::DIV) return BuildAsm({"DIV.F"});
        }
        else if (e.dataType == DataType::DOUBLE)
        {
            if      (e.type == expr_type::ADD) return BuildAsm({"ADD.F64"});
            else if (e.type == expr_type::SUB) return BuildAsm({"SUB.F64"});
            else if (e.type == expr_type::MUL) return BuildAsm({"MUL.F64"});
            else if (e.type == expr_type::DIV) return BuildAsm({"DIV.F64"});
        }

        RETURN_ERROR();
    }

    std::string StackCodeGenerator::expr_comparison(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string StackCodeGenerator::expr_func_call(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    //---- RegisterCodeGenerator Implementation ----//
  
    std::string RegisterCodeGenerator::stmt_assignment(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string RegisterCodeGenerator::stmt_initialization(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string RegisterCodeGenerator::stmt_func_call(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string RegisterCodeGenerator::stmt_branch(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string RegisterCodeGenerator::stmt_creation(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string RegisterCodeGenerator::stmt_destruction(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }


    std::string RegisterCodeGenerator::expr_id(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string RegisterCodeGenerator::expr_literal(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string RegisterCodeGenerator::expr_arithmetic(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string RegisterCodeGenerator::expr_comparison(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    std::string RegisterCodeGenerator::expr_func_call(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

}
