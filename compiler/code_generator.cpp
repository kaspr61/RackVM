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
#define RETURN_ERROR() {m_hasError = true; WriteAsm({"error ->", __FUNCTION__ });}

namespace Compiler
{
    //---- CodeGenerator Implementation ----//

    CodeGenerator::CodeGenerator(std::ostream& output) :
        m_ss(), 
        m_hasError(false), 
        m_nextLabel(0), 
        m_elseLabel(""), 
        m_endLabel(""), 
        m_ifLabel(""), 
        m_out(output),
        m_lastInstr()
        {
        }

    CodeGenerator::~CodeGenerator() 
    {
    }

    bool CodeGenerator::TranslateFunctions(const std::vector<func>& funcList)
    {
        auto funcIt = funcList.begin();
        while (funcIt != funcList.end())
        {
            WriteLabel(funcIt->id.id);

            if (funcIt->argVarCnt > 0)
            {
                // First, load up the argument variable values, which are consumed from the stack.
                // It is important to load them in a reversed order, because... stack.
                for (auto argIt = funcIt->args.rbegin(); argIt != funcIt->args.rend(); ++argIt)
                {
                    if (GetDataTypeWords(argIt->id.dataType) == 2)
                        WriteAsm({"STA.64", STR(argIt->id.position)});
                    else
                        WriteAsm({"STA", STR(argIt->id.position)});
                }
            }

            // Translate the contained statements of the function.
            auto stmtIt = funcIt->statements.begin();
            while (stmtIt != funcIt->statements.end())
                TranslateStatement(*stmtIt++);

            ++funcIt;
        }

        return !m_hasError;
    }

    bool CodeGenerator::TranslateStatement(const stmt& s)
    {
        std::vector<stmt>::const_iterator substmt;

        switch (s.type)
        {
            case stmt_type::ASSIGNMENT:
            case stmt_type::INITIALIZATION:
            case stmt_type::ASSIGN_OFFSET: stmt_assignment(s);
                break;

            case stmt_type::FUNC_CALL: stmt_func_call(s);
                break;

            case stmt_type::BRANCH: stmt_branch(s);
                break;

            case stmt_type::BLOCK: 
                substmt = s.substmts.begin();
                while (substmt != s.substmts.end())
                    TranslateStatement(*substmt++);
                break;

            case stmt_type::CREATION: stmt_creation(s);
                break;

            case stmt_type::DESTRUCTION: stmt_destruction(s);
                break;

            case stmt_type::RETURN: stmt_return(s);
                break;

            default: break;
        }

        return !m_hasError;
    }

    bool CodeGenerator::TranslateExpression(const expr& e)
    {
        switch (e.type)
        {
            case expr_type::ID: expr_id(e);
                break;

            case expr_type::ID_OFFSET: expr_id_offset(e);
                break;

            case expr_type::NUMBER:
            case expr_type::STRING: expr_literal(e);
                break;

            case expr_type::ADD:
            case expr_type::SUB:
            case expr_type::MUL:
            case expr_type::DIV: expr_arithmetic(e);
                break;

            case expr_type::OR:
            case expr_type::AND: expr_logical(e);
                break;

            case expr_type::NEG: expr_unary(e);
                break;

            case expr_type::CALL: expr_func_call(e);
                break;

            case expr_type::EQ:
            case expr_type::NEQ:
            case expr_type::GT:
            case expr_type::LT:
            case expr_type::GEQ:
            case expr_type::LEQ: expr_comparison(e);
                break;
        }

        return !m_hasError;
    }

    std::string CodeGenerator::BuildAsm(std::initializer_list<std::string>& operands)
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

    StackCodeGenerator::StackCodeGenerator(std::ostream& output) :
        CodeGenerator(output)
    {
    }

    StackCodeGenerator::~StackCodeGenerator()
    {
    }

    void StackCodeGenerator::stmt_assignment(const stmt& s)
    {
        TranslateExpression(s.expressions.front());
        
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

        WriteAsm({instr, STR(s.id.position)});
    }

    void StackCodeGenerator::stmt_func_call(const stmt& s)
    {
        TranslateExpression(s.expressions.front());
    }

    void StackCodeGenerator::stmt_branch(const stmt& s)
    {
        const stmt& ifStmt = s.substmts.front();
        const expr& ifCond = ifStmt.expressions.front();

        m_elseLabel = CreateLabel();
        m_ifLabel = CreateLabel();            

        if (s.substmts.size() == 2 && m_endLabel == "")
            m_endLabel = CreateLabel();
        else if (s.substmts.size() == 1 && m_endLabel == "")
            m_endLabel = m_elseLabel;

        if (s.substmts.size() == 1)
            m_elseLabel = m_endLabel;

        TranslateExpression(ifCond);

        if (ifCond.type == expr_type::OR && s.substmts.size() == 2)
            WriteAsm({"JMP", m_elseLabel});

        WriteLabel(m_ifLabel);

        auto stmtIt = ifStmt.substmts.begin();
        while (stmtIt != ifStmt.substmts.end())
            TranslateStatement(*stmtIt++);

        if (s.substmts.size() == 2)
            WriteAsm({"JMP", m_endLabel});

        WriteLabel(m_elseLabel);

        if (s.substmts.size() < 2)
            return;

        const stmt& elseStmt = s.substmts.back();

        stmtIt = elseStmt.substmts.begin();
        while (stmtIt != elseStmt.substmts.end())
            TranslateStatement(*stmtIt++);

        if (m_endLabel != "" && m_lastInstr != m_endLabel)
            WriteLabel(m_endLabel);

        m_endLabel = "";
        m_elseLabel = "";
        m_ifLabel = "";
    }

    void StackCodeGenerator::stmt_creation(const stmt& s)
    {
        TranslateExpression(s.expressions.front());

        WriteAsm({"NEW"});

        if (s.id.type == identifier_type::ARG_VAR)
            WriteAsm({"STA", STR(s.id.position)});
        else if (s.id.type == identifier_type::LOCAL_VAR)
            WriteAsm({"STL", STR(s.id.position)});
        else
            RETURN_ERROR();
    }

    void StackCodeGenerator::stmt_destruction(const stmt& s)
    {
        TranslateExpression(s.expressions.front());

        WriteAsm({"DEL"});
    }

    void StackCodeGenerator::stmt_return(const stmt& s)
    {
        if (s.expressions.size() > 0)
            TranslateExpression(s.expressions.front());

        WriteAsm({"RET"});
    }


    void StackCodeGenerator::expr_id(const expr& e)
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

        WriteAsm({instr, STR(e.id.position)});
    }

    void StackCodeGenerator::expr_id_offset(const expr& e)
    {
        TranslateExpression(e.operands.front());
        TranslateExpression(e.operands.back());

        WriteAsm({"ADD"});
        
        if (GetDataTypeWords(e.dataType) == 2) // If array of 64-bit values.
            WriteAsm({"LDM.64"});
        else
            WriteAsm({"LDM"});
    }

    void StackCodeGenerator::expr_literal(const expr& e)
    {
        if (e.dataType == DataType::INT)
            WriteAsm({"LDI", STR(e.intValue)});
        else if (e.dataType == DataType::LONG)
            WriteAsm({"LDI.64", STR(e.longValue)});
        else
            RETURN_ERROR();
    }

    void StackCodeGenerator::expr_arithmetic(const expr& e)
    {
        TranslateExpression(e.operands.front());
        TranslateExpression(e.operands.back());

        std::string instr;

        if      (e.type == expr_type::ADD) instr = "ADD";
        else if (e.type == expr_type::SUB) instr = "SUB";
        else if (e.type == expr_type::MUL) instr = "MUL";
        else if (e.type == expr_type::DIV) instr = "DIV";
        else    RETURN_ERROR();

        if      (e.dataType == DataType::FLOAT)  instr.append(".F");
        else if (e.dataType == DataType::DOUBLE) instr.append(".F64");
        else if (e.dataType == DataType::LONG)   instr.append(".64");

        WriteAsm({instr});
    }

    void StackCodeGenerator::expr_comparison(const expr& e)
    {
        TranslateExpression(e.operands.front());
        TranslateExpression(e.operands.back());

        std::string instr;
        if      (e.type == expr_type::EQ)  instr = "CPEQ";
        else if (e.type == expr_type::NEQ) instr = "CPNQ";
        else if (e.type == expr_type::GT)  instr = "CPGT";
        else if (e.type == expr_type::LT)  instr = "CPLT";
        else if (e.type == expr_type::GEQ) instr = "CPGQ";
        else if (e.type == expr_type::LEQ) instr = "CPLQ";
        else    RETURN_ERROR();

        if      (e.dataType == DataType::FLOAT)  instr.append(".F");
        else if (e.dataType == DataType::DOUBLE) instr.append(".F64");
        else if (e.dataType == DataType::LONG)   instr.append(".64");
        
        WriteAsm({instr});
    }

    void StackCodeGenerator::expr_logical(const expr& e)
    {
        TranslateExpression(e.operands.front());

        if (e.type == expr_type::OR && m_ifLabel != "" && !GetLastInstrIsBranch()) 
            WriteAsm({"BRNZ",  m_ifLabel, "; ", __FUNCTION__, "1"});
        else if (e.type == expr_type::AND && m_elseLabel != "" && !GetLastInstrIsBranch()) 
            WriteAsm({"BRZ", m_elseLabel, "; ", __FUNCTION__, "2"});

        TranslateExpression(e.operands.back());

        if (e.type == expr_type::OR && m_ifLabel != "" && !GetLastInstrIsBranch())
            WriteAsm({"BRNZ", m_ifLabel, "; ", __FUNCTION__, "3"});
        else if (e.type == expr_type::AND && m_elseLabel != "" && !GetLastInstrIsBranch())
            WriteAsm({"BRZ", m_elseLabel, "; ", __FUNCTION__, "4"});

        if (m_elseLabel == "") // If this expr is NOT part of a branch condition.
        {
            if (e.type == expr_type::OR)
                WriteAsm({"OR"});
            else if (e.type == expr_type::AND)
                WriteAsm({"AND"});
            else
                RETURN_ERROR();
        }
    }

    void StackCodeGenerator::expr_func_call(const expr& e)
    {
        const expr& func = e.operands.front();
        const expr& args = e.operands.back();
        auto it = args.operands.begin();
        while (it != args.operands.end())
            TranslateExpression(*it++);
        
        WriteAsm({"CALL", func.id.id});
    }

    void StackCodeGenerator::expr_unary(const expr& e)
    {
        std::string instr;

        if      (e.type == expr_type::NEG) instr = "NEG";
        else    RETURN_ERROR();

        if      (e.dataType == DataType::FLOAT)  instr.append(".F");
        else if (e.dataType == DataType::DOUBLE) instr.append(".F64");
        else if (e.dataType == DataType::LONG)   instr.append(".64");
        
        WriteAsm({instr});
    }

    //---- RegisterCodeGenerator Implementation ----//
  
    RegisterCodeGenerator::RegisterCodeGenerator(std::ostream& output) :
        CodeGenerator(output)
    {
    }

    RegisterCodeGenerator::~RegisterCodeGenerator()
    {
    }

    void RegisterCodeGenerator::stmt_assignment(const stmt& s)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_func_call(const stmt& s)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_branch(const stmt& s)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_creation(const stmt& s)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_destruction(const stmt& s)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::stmt_return(const stmt& s)
    {
        RETURN_ERROR();
    }


    void RegisterCodeGenerator::expr_id(const expr& e)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_id_offset(const expr& e)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_literal(const expr& e)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_arithmetic(const expr& e)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_logical(const expr& e)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_comparison(const expr& e)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_func_call(const expr& e)
    {
        RETURN_ERROR();
    }

    void RegisterCodeGenerator::expr_unary(const expr& e)
    {
        RETURN_ERROR();
    }

}
