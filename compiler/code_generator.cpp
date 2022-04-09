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
#define RETURN_ERROR() {m_hasError = true; AddInstruction({"NOP", "; error ->", __FUNCTION__ });}

namespace Compiler
{
    static constexpr uint32_t DEFAULT_HEAP_SIZE     = 64;     // 64 KiB
    static constexpr uint32_t DEFAULT_MAX_HEAP_SIZE = 262144; // 256 MiB = 262 144 KiB

    static const std::initializer_list<std::string> g_sysFuncs =
    {
        "print",
        "input",
        "write",
        "read",
        "open",
        "close",
        "str"
    };

    bool IsSystemFunction(const std::string& id)
    {
        return std::find(g_sysFuncs.begin(), g_sysFuncs.end(), id) != g_sysFuncs.end();
    }

    bool HasVariadicArguments(const func& function)
    {
        auto varArg = std::find_if(function.args.begin(), function.args.end(), 
            [](const stmt& arg) { return arg.id.type == identifier_type::VARIADIC_ARG; }); 

        return varArg != function.args.end();
    }

    //---- CodeGenerator Implementation ----//

    CodeGenerator::CodeGenerator(uint32_t initialHeapSize, uint32_t maxHeapSize, 
        const std::vector<func>& funcList, const StringLiteralMap& literals) :
        m_ss(), 
        m_hasError(false), 
        m_nextLabel(0), 
        m_currFunc(""),
        m_lastInstr(),
        m_funcList(funcList),
        m_literals(literals)
        {
            m_initHeapSize = initialHeapSize != 0 ? initialHeapSize : DEFAULT_HEAP_SIZE;
            m_maxHeapSize = maxHeapSize != 0 ? maxHeapSize : DEFAULT_MAX_HEAP_SIZE;
        }

    CodeGenerator::~CodeGenerator() 
    {
    }

    bool CodeGenerator::TranslateFunctions()
    {
        for (auto funcIt = m_funcList.begin(); funcIt != m_funcList.end(); ++funcIt)
        {
            const std::string& funcName = funcIt->id.id;
            if (funcName.size() > 2 && funcName[0] == '_' && funcName[1] == '_')
                continue;

            m_currFunc = funcName;
            m_instr.emplace(m_currFunc, std::vector<Instruction>());

            if (funcIt->argVarCnt > 0)
            {
                // First, load up the argument variable values, which are consumed from the stack.
                // It is important to load them in a reversed order, because... stack.
                for (auto argIt = funcIt->args.rbegin(); argIt != funcIt->args.rend(); ++argIt)
                {
                    if (GetDataTypeBytes(argIt->id.dataType) == 8)
                        AddInstruction({"STA.64", STR(argIt->id.position)});
                    else
                        AddInstruction({"STA", STR(argIt->id.position)});
                }
            }

            // Translate the contained statements of the function.
            auto stmtIt = funcIt->statements.begin();
            while (stmtIt != funcIt->statements.end())
                TranslateStatement(*stmtIt++);

            m_instr[m_currFunc].front().label = m_currFunc;
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
            case expr_type::LEQ:
            case expr_type::STREQ: expr_comparison(e);
                break;

            case expr_type::CAST: expr_cast(e);
                break;
        }

        return !m_hasError;
    }

    std::string CodeGenerator::BuildAsm(const std::vector<std::string>& operands)
    {
        if (operands.size() < 1)
            return std::string("error: ").append(__FUNCTION__);

        m_ss.clear();
        m_ss.str("");
        
        // Format a line representing an assembly instruction.
        auto it = operands.begin();
        m_ss << "  " << std::setw(6) << std::left << *it++; // First operand (the mnemonic) has width 6
        
        while (it != operands.end())
        {
            std::string arg = *it++;
            if (it != operands.end())
                arg.append(",");

            m_ss << "  " << std::setw(8) << std::left << arg; // Other operands have width 8
        }

        return m_ss.str();
    }

    void CodeGenerator::Flush(std::ostream& output)
    {
        // First, write the header.
        if (typeid(*this) == typeid(StackCodeGenerator))
            output << std::left << std::setw(10) << ".MODE" << std::setw(14) << "Stack" << "; Use the stack instruction set." << std::endl;
        else
            output << std::setw(10) << ".MODE" << std::setw(14) << "Register" << "; Use the register instruction set." << std::endl;

        output << std::setw(10) << ".HEAP" << std::setw(14) << m_initHeapSize << "; KiB" << std::endl;
        output << std::setw(10) << ".HEAP_MAX" << std::setw(14) << m_maxHeapSize << "; KiB" << std::endl;
        output << std::endl;

        // Start on the actual instructions.
        for (auto& map : m_instr)
        {
            // Do a first pass to add labels to the instructions that
            // are pointed to by next or cond.
            for (auto instr = map.second.begin(); instr != map.second.end(); ++instr)
            {
                // This instruction refers to the instruction AFTER what it is pointing to.
                if (instr->jumpAfter > -1)
                {
                    auto isJumpAfter = [&](Instruction& i) { return &i == &map.second[instr->jumpTo]; };

                    // Get the iterator to the instruction directly after what instr->jumpAfter points to.
                    if (instr->jumpAfter + 1 < map.second.size())
                    {
                        auto& next = map.second[instr->jumpAfter + 1];
                        // Set up label, if there is none.
                        if (next.label == "")
                            next.label = std::move(CreateLabel());

                        // Append the label of the instruction directly AFTER what jumpAfter points to.
                        instr->operands.push_back(next.label);
                    }
                    else
                        std::cerr << "Tried to jump outside function." << std::endl;
                }
                // This instruction refers to the instruction jumpTo is pointing to.
                else if (instr->jumpTo > -1)
                {
                    if (map.second[instr->jumpTo].label == "")
                        map.second[instr->jumpTo].label = std::move(CreateLabel());

                    // Append the label of the instruction that jumpTo points to.
                    instr->operands.push_back(map.second[instr->jumpTo].label);
                }
            }

            // Do a second pass to actually flush the instruction as text to 'output'.
            for (auto& instr : map.second)
            {
                // If instruction has a label, output that first.
                if (instr.label != "")
                    output << instr.label << ':' << std::endl;

                output << BuildAsm(instr.operands) << std::endl;
            }
        }

        output << std::endl << "; LITERALS" << std::endl;

        // Create all string literals at the end of the program.
        for (auto literal : m_literals)
        {
            output << literal.second << ':' << std::endl;
            output << BuildAsm({".BYTE", STR(literal.first.size()+1), '\"'+literal.first+'\"'}) << std::endl;
        }
    }

    //---- StackCodeGenerator Implementation ----//

    StackCodeGenerator::StackCodeGenerator(uint32_t initialHeapSize, uint32_t maxHeapSize, 
        const std::vector<func>& funcList, const StringLiteralMap& literals) :
        CodeGenerator(initialHeapSize, maxHeapSize, funcList, literals)
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

        if (GetDataTypeBytes(s.id.dataType) == 8) // If 64-bit data type.
            instr.append(".64");

        AddInstruction({instr, STR(s.id.position)});
    }

    void StackCodeGenerator::stmt_func_call(const stmt& s)
    {
        TranslateExpression(s.expressions.front());
    }

    void StackCodeGenerator::stmt_branch(const stmt& s)
    {
        const stmt& ifStmt = s.substmts.front();
        const expr& ifCond = ifStmt.expressions.front();

        TranslateExpression(ifCond);
        
        // Always try to branch directly after the condition expression.
        int32_t condBranchInstr = AddInstruction({"BRZ"}); // Should jump to else block, or to end, if none exists.

        // Here starts the if block.

        auto stmtIt = ifStmt.substmts.begin();
        while (stmtIt != ifStmt.substmts.end())
            TranslateStatement(*stmtIt++);

        int32_t jumpToEnd = -1;
        if (s.substmts.size() == 2)
        {
            jumpToEnd = AddInstruction({"JMP"}); // Should jump to the end.
            GetFuncInstr(condBranchInstr)->jumpAfter = m_lastInstr; // Should jump to else.
        }
        else // Handle when there is no else statement.
        {
            // If not already set, then set conditional branch to jump to the 
            // instruction AFTER the last instruction made, i.e. the end.
            GetFuncInstr(condBranchInstr)->jumpAfter = m_lastInstr;
            return;
        }

        // Here starts the else block.

        const stmt& elseStmt = s.substmts.back();

        stmtIt = elseStmt.substmts.begin();
        while (stmtIt != elseStmt.substmts.end())
            TranslateStatement(*stmtIt++);

        // Set the instruction 'jumpToEnd' to jump to the instruction after the last instruction.
        // Labels are created and resolved when calling Flush().
        GetFuncInstr(jumpToEnd)->jumpAfter = m_lastInstr;
    }

    void StackCodeGenerator::stmt_creation(const stmt& s)
    {
        TranslateExpression(s.expressions.front());

        AddInstruction({"NEW"});

        if (s.id.type == identifier_type::ARG_VAR)
            AddInstruction({"STA", STR(s.id.position)});
        else if (s.id.type == identifier_type::LOCAL_VAR)
            AddInstruction({"STL", STR(s.id.position)});
        else
            RETURN_ERROR();
    }

    void StackCodeGenerator::stmt_destruction(const stmt& s)
    {
        TranslateExpression(s.expressions.front());

        AddInstruction({"DEL"});
    }

    void StackCodeGenerator::stmt_return(const stmt& s)
    {
        if (s.expressions.size() > 0)
            TranslateExpression(s.expressions.front());

        AddInstruction({"RET"});
    }


    void StackCodeGenerator::expr_id(const expr& e)
    {
        identifier_type idType = e.id.type;
        int32_t dataTypeSize = GetDataTypeBytes(e.dataType);
        std::string instr;

        if (idType == identifier_type::ARG_VAR)
            instr = "LDA";
        else if (idType == identifier_type::LOCAL_VAR)
            instr = "LDL";
        else
            RETURN_ERROR();

        if (dataTypeSize == 8) // If 64-bit data type.
            instr.append(".64");

        AddInstruction({instr, STR(e.id.position)});
    }

    void StackCodeGenerator::expr_id_offset(const expr& e)
    {
        TranslateExpression(e.operands.front());
        
        const expr& index = e.operands.back();
        if (index.intValue > 0 || index.type != expr_type::NUMBER)
        {
            TranslateExpression(index);
            AddInstruction({"ADD"});
        }
        
        // Don't load from mem if it's a string. Strings are just pointers.
        if (e.operands.front().dataType != DataType::STRING) 
        {
            if (GetDataTypeBytes(e.dataType) == 8) // If array of 64-bit values.
                AddInstruction({"LDM.64"});
            else
                AddInstruction({"LDM"});
        }
    }

    void StackCodeGenerator::expr_literal(const expr& e)
    {
        if (e.dataType == DataType::INT)
        {
            AddInstruction({"LDI", STR(e.intValue)});
        }
        else if (e.dataType == DataType::LONG)
        {
            AddInstruction({"LDI.64", STR(e.longValue)});
        }
        else if (e.dataType == DataType::STRING)
        {
            const auto it = m_literals.find(e.strValue);
            if (it != m_literals.end())
                AddInstruction({"STR", it->second});
            else
                RETURN_ERROR();
        }
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

        AddInstruction({instr});
    }

    void StackCodeGenerator::expr_comparison(const expr& e)
    {
        TranslateExpression(e.operands.front());
        TranslateExpression(e.operands.back());

        const expr& lhs = e.operands.front();
        if (lhs.dataType == DataType::STRING)
        {
            if (e.operands.back().dataType != DataType::STRING)
                RETURN_ERROR();

            switch(e.type)
            {
                case expr_type::EQ:    AddInstruction({"CPSTR"}); break;
                case expr_type::NEQ:   AddInstruction({"CPSTR"}); AddInstruction({"CPZ"}); break;
                case expr_type::STREQ: AddInstruction({"CPCHR"}); break;
                default:               RETURN_ERROR();
            }

            return;
        }

        std::string instr;
        if      (e.type == expr_type::EQ)    instr = "CPEQ";
        else if (e.type == expr_type::NEQ)   instr = "CPNQ";
        else if (e.type == expr_type::GT)    instr = "CPGT";
        else if (e.type == expr_type::LT)    instr = "CPLT";
        else if (e.type == expr_type::GEQ)   instr = "CPGQ";
        else if (e.type == expr_type::LEQ)   instr = "CPLQ";
        else    RETURN_ERROR();

        if      (e.dataType == DataType::FLOAT)  instr.append(".F");
        else if (e.dataType == DataType::DOUBLE) instr.append(".F64");
        else if (e.dataType == DataType::LONG)   instr.append(".64");
        
        AddInstruction({instr});
    }

    void StackCodeGenerator::expr_logical(const expr& e)
    {
        TranslateExpression(e.operands.front());
        TranslateExpression(e.operands.back());

        if (e.type == expr_type::OR)
            AddInstruction({"OR"});
        else if (e.type == expr_type::AND)
            AddInstruction({"AND"});
        else
            RETURN_ERROR();
    }

    void StackCodeGenerator::expr_func_call(const expr& e)
    {
        const expr& func = e.operands.front();
        const expr& args = e.operands.back();

        auto funcIt = std::find_if(m_funcList.begin(), m_funcList.end(),
            [=](const Compiler::func& f) { return f.id.id == func.id.id;});

        bool hasVariadicArgs = false;
        if (funcIt != m_funcList.end())
            hasVariadicArgs = HasVariadicArguments(*funcIt);

        for (auto it = args.operands.begin(); it != args.operands.end(); ++it)
        {
            TranslateExpression(*it);

            if (hasVariadicArgs)
                AddInstruction({"SARG", STR(GetDataTypeBytes(it->dataType))});
        }

        std::string label = func.id.id;
        if (label[0] = '_' && label[1] == '_')
            AddInstruction({"SCALL", label});
        else
            AddInstruction({"CALL", label});
    }

    void StackCodeGenerator::expr_unary(const expr& e)
    {
        TranslateExpression(e.operands.front());
        DataType dataType = e.operands.front().dataType;

        std::string instr;

        if      (e.type == expr_type::NEG) instr = "NEG";
        else    RETURN_ERROR();

        if      (dataType == DataType::FLOAT)  instr.append(".F");
        else if (dataType == DataType::DOUBLE) instr.append(".F64");
        else if (dataType == DataType::LONG)   instr.append(".64");
        
        AddInstruction({instr});
    }

    void StackCodeGenerator::expr_cast(const expr& e)
    {
        TranslateExpression(e.operands.front());

        DataType from = e.operands.front().dataType;
        DataType to = e.dataType;
        std::string instr = "";
        std::string arg = "";
        const expr* argExpr = nullptr;
        if (&e.operands.back() != &e.operands.front()) // If an arg was given.
            argExpr = &e.operands.back();

        if (from == DataType::INT)
        {
            switch (to)
            {
                case DataType::INT:    return; // Ignore
                case DataType::LONG:   instr = "ITOL"; break;
                case DataType::FLOAT:  instr = "ITOF"; break;
                case DataType::DOUBLE: instr = "ITOD"; break;
                case DataType::STRING: instr = "ITOS"; break;
            }
        }
        else if (from == DataType::LONG)
        {
            switch (to)
            {
                case DataType::LONG:   return; // Ignore
                case DataType::INT:    instr = "LTOI"; break;
                case DataType::FLOAT:  instr = "LTOF"; break;
                case DataType::DOUBLE: instr = "LTOD"; break;
                case DataType::STRING: instr = "LTOS"; break;
            }
        }
        else if (from == DataType::FLOAT)
        {
            switch (to)
            {
                case DataType::FLOAT:  return; // Ignore
                case DataType::INT:    instr = "FTOI"; break;
                case DataType::LONG:   instr = "FTOL"; break;
                case DataType::DOUBLE: instr = "FTOD"; break;
                case DataType::STRING: instr = "FTOS";
                    arg = argExpr ? std::to_string(argExpr->intValue) : "255"; // Set to VM default.
                    break;
            }
        }
        else if (from == DataType::DOUBLE)
        {
            switch (to)
            {
                case DataType::DOUBLE: return; // Ignore
                case DataType::INT:    instr = "DTOI"; break;
                case DataType::FLOAT:  instr = "DTOF"; break;
                case DataType::LONG:   instr = "DTOL"; break;
                case DataType::STRING: instr = "DTOS";
                    arg = argExpr ? std::to_string(argExpr->intValue) : "255"; // Set to VM default.
                    break;
            }
        }
        else if (from == DataType::STRING)
        {
            // Needs to process arg if present, otherwise use a default.
            switch (to)
            {
                // Special case: if explicity casting a string to a string, then copy it into a new string.
                // The arg specifies how many characters to copy. This can be used for substring functionality.
                case DataType::STRING: instr = "STRCPY";
                    arg = argExpr ? std::to_string(argExpr->intValue+1) : "2147483647";
                    break; 

                case DataType::INT: instr = "STOI"; 
                    arg = argExpr ? std::to_string(argExpr->intValue) : "0";
                    break;

                case DataType::FLOAT: instr = "STOF";
                    arg = argExpr ? std::to_string(argExpr->floatValue) : "0.0f";
                    break;

                case DataType::LONG: instr = "STOL";
                    arg = argExpr ? std::to_string(argExpr->longValue) : "0";
                    break;

                case DataType::DOUBLE: instr = "STOD";
                    arg = argExpr ? std::to_string(argExpr->doubleValue) : "0.0";
                    break;
            }
        }

        if (instr != "")
        {
            if (arg == "")
                AddInstruction({instr});
            else
                AddInstruction({instr, arg});
        }
        else
        {
            RETURN_ERROR();   
        }
    }

    //---- RegisterCodeGenerator Implementation ----//
  
    RegisterCodeGenerator::RegisterCodeGenerator(uint32_t initialHeapSize, uint32_t maxHeapSize, 
        const std::vector<func>& funcList, const StringLiteralMap& literals) :
        CodeGenerator(initialHeapSize, maxHeapSize, funcList, literals)
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

    void RegisterCodeGenerator::expr_cast(const expr& e)
    {
        RETURN_ERROR();
    }
}
