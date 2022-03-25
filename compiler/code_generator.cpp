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
#define RETURN_ERROR() {m_hasError = true; output << "error -> " << __FUNCTION__ << std::endl;}

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

            if (funcIt->argVarCnt > 0)
            {
                // First, load up the argument variable values, which are consumed from the stack.
                // It is important to load them in a reversed order, because... stack.
                for (auto argIt = funcIt->args.rbegin(); argIt != funcIt->args.rend(); ++argIt)
                {
                    if (GetDataTypeWords(argIt->id.dataType) == 2)
                        output << BuildAsm({"STA.64", STR(argIt->id.position)}) << std::endl;
                    else
                        output << BuildAsm({"STA", STR(argIt->id.position)}) << std::endl;
                }
            }

            // Translate the contained statements of the function.
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
            case stmt_type::INITIALIZATION:
            case stmt_type::ASSIGN_OFFSET: stmt_assignment(s, output);
                break;

            case stmt_type::FUNC_CALL: stmt_func_call(s, output);
                break;

            case stmt_type::BRANCH: stmt_branch(s, output);
                break;

            case stmt_type::BLOCK: 
                substmt = s.substmts.begin();
                while (substmt != s.substmts.end())
                    TranslateStatement(*substmt++, output);
                break;

            case stmt_type::CREATION: stmt_creation(s, output);
                break;

            case stmt_type::DESTRUCTION: stmt_destruction(s, output);
                break;

            case stmt_type::RETURN: stmt_return(s, output);
                break;

            default: break;
        }

        return !m_hasError;
    }

    bool CodeGenerator::TranslateExpression(const expr& e, std::ostream& output)
    {
        switch (e.type)
        {
            case expr_type::ID: expr_id(e, output);
                break;

            case expr_type::ID_OFFSET: expr_id_offset(e, output);
                break;

            case expr_type::NUMBER:
            case expr_type::STRING: expr_literal(e, output);
                break;

            case expr_type::ADD:
            case expr_type::SUB:
            case expr_type::MUL:
            case expr_type::DIV: expr_arithmetic(e, output);
                break;

            case expr_type::CALL: expr_func_call(e, output);
                break;

            case expr_type::EQ:
            case expr_type::NEQ:
            case expr_type::GT:
            case expr_type::LT:
            case expr_type::GEQ:
            case expr_type::LEQ: expr_comparison(e, output);
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

    void StackCodeGenerator::stmt_assignment(const stmt& s, std::ostream& output)
    {
        TranslateExpression(s.expressions.front(), output);
        
        identifier_type idType = s.id.type;
        std::string instr;
        if (idType == identifier_type::ARG_VAR)
            instr = "STA";
        else if (idType == identifier_type::LOCAL_VAR)
            instr = "STL";
        else
            RETURN_ERROR();

        if (GetDataTypeWords(s.id.dataType) == 2) // If 64-bit data type.
            instr.append(".64");

        output << BuildAsm({instr, STR(s.id.position)}) << std::endl;
    }

    void StackCodeGenerator::stmt_func_call(const stmt& s, std::ostream& output)
    {
        TranslateExpression(s.expressions.front(), output);
    }

    void StackCodeGenerator::stmt_branch(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void StackCodeGenerator::stmt_creation(const stmt& s, std::ostream& output)
    {
        TranslateExpression(s.expressions.front(), output);

        output << BuildAsm({"NEW"}) << std::endl;

        if (s.id.type == identifier_type::ARG_VAR)
            output << BuildAsm({"STA", STR(s.id.position)}) << std::endl;
        else if (s.id.type == identifier_type::LOCAL_VAR)
            output << BuildAsm({"STL", STR(s.id.position)}) << std::endl;
        else
            RETURN_ERROR();
    }

    void StackCodeGenerator::stmt_destruction(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void StackCodeGenerator::stmt_return(const stmt& s, std::ostream& output)
    {
        if (s.expressions.size() > 0)
            TranslateExpression(s.expressions.front(), output);

        output << BuildAsm({"RET"}) << std::endl;
    }


    void StackCodeGenerator::expr_id(const expr& e, std::ostream& output)
    {
        identifier_type idType = e.id.type;
        int32_t dataTypeWords = GetDataTypeWords(e.dataType);
        std::string instr;

        if (idType == identifier_type::ARG_VAR)
            instr = "LDA";
        else if (idType == identifier_type::LOCAL_VAR)
            instr = "LDL";
        else
            RETURN_ERROR();

        if (dataTypeWords == 2) // If 64-bit data type.
            instr.append(".64");

        output << BuildAsm({instr, STR(e.id.position)}) << std::endl;
    }

    void StackCodeGenerator::expr_id_offset(const expr& e, std::ostream& output)
    {
        TranslateExpression(e.operands.front(), output);
        TranslateExpression(e.operands.back(), output);

        output << BuildAsm({"ADD"}) << std::endl;
        
        if (GetDataTypeWords(e.dataType) == 2) // If array of 64-bit values.
            output << BuildAsm({"LDM.64"}) << std::endl;
        else
            output << BuildAsm({"LDM"}) << std::endl;
    }

    void StackCodeGenerator::expr_literal(const expr& e, std::ostream& output)
    {
        if (e.dataType == DataType::INT)
            output << BuildAsm({"LDI", STR(e.intValue)}) << std::endl;
        else if (e.dataType == DataType::LONG)
            output << BuildAsm({"LDI.64", STR(e.longValue)}) << std::endl;
        else
            RETURN_ERROR();
    }

    void StackCodeGenerator::expr_arithmetic(const expr& e, std::ostream& output)
    {
        TranslateExpression(e.operands.front(), output);
        TranslateExpression(e.operands.back(), output);

        if (e.dataType == DataType::INT)
        {
            if      (e.type == expr_type::ADD) output << BuildAsm({"ADD"}) << std::endl;
            else if (e.type == expr_type::SUB) output << BuildAsm({"SUB"}) << std::endl;
            else if (e.type == expr_type::MUL) output << BuildAsm({"MUL"}) << std::endl;
            else if (e.type == expr_type::DIV) output << BuildAsm({"DIV"}) << std::endl;
        }
        else if (e.dataType == DataType::LONG)
        {
            if      (e.type == expr_type::ADD) output << BuildAsm({"ADD.64"}) << std::endl;
            else if (e.type == expr_type::SUB) output << BuildAsm({"SUB.64"}) << std::endl;
            else if (e.type == expr_type::MUL) output << BuildAsm({"MUL.64"}) << std::endl;
            else if (e.type == expr_type::DIV) output << BuildAsm({"DIV.64"}) << std::endl;
        }
        else if (e.dataType == DataType::FLOAT)
        {
            if      (e.type == expr_type::ADD) output << BuildAsm({"ADD.F"}) << std::endl;
            else if (e.type == expr_type::SUB) output << BuildAsm({"SUB.F"}) << std::endl;
            else if (e.type == expr_type::MUL) output << BuildAsm({"MUL.F"}) << std::endl;
            else if (e.type == expr_type::DIV) output << BuildAsm({"DIV.F"}) << std::endl;
        }
        else if (e.dataType == DataType::DOUBLE)
        {
            if      (e.type == expr_type::ADD) output << BuildAsm({"ADD.F64"}) << std::endl;
            else if (e.type == expr_type::SUB) output << BuildAsm({"SUB.F64"}) << std::endl;
            else if (e.type == expr_type::MUL) output << BuildAsm({"MUL.F64"}) << std::endl;
            else if (e.type == expr_type::DIV) output << BuildAsm({"DIV.F64"}) << std::endl;
        }
        else
            RETURN_ERROR();
    }

    void StackCodeGenerator::expr_comparison(const expr& e, std::ostream& output)
    {
        TranslateExpression(e.operands.front(), output);
        TranslateExpression(e.operands.back(), output);

        std::string instr;
        if      (e.type == expr_type::EQ)  instr = "CPEQ";
        else if (e.type == expr_type::NEQ) instr = "CPNQ";
        else if (e.type == expr_type::GT)  instr = "CPGT";
        else if (e.type == expr_type::LT)  instr = "CPLT";
        else if (e.type == expr_type::GEQ) instr = "CPGQ";
        else if (e.type == expr_type::LEQ) instr = "CPLQ";
        else    RETURN_ERROR();

        if (GetDataTypeWords(e.dataType) == 2) // If 64-bit data type.
            instr.append(".64");
        
        output << BuildAsm({instr}) << std::endl;
    }

    void StackCodeGenerator::expr_func_call(const expr& e, std::ostream& output)
    {
        const expr& func = e.operands.front();
        const expr& args = e.operands.back();
        auto it = args.operands.begin();
        while (it != args.operands.end())
            TranslateExpression(*it++, output);
        
        output << BuildAsm({"CALL", func.id.id}) << std::endl;
    }

    //---- RegisterCodeGenerator Implementation ----//
  
    void RegisterCodeGenerator::stmt_assignment(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_func_call(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_branch(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_creation(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_destruction(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_return(const stmt& s, std::ostream& output)
    {
        RETURN_ERROR();
    }


    void RegisterCodeGenerator::expr_id(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_id_offset(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_literal(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_arithmetic(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_comparison(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_func_call(const expr& e, std::ostream& output)
    {
        RETURN_ERROR();
    }

}
