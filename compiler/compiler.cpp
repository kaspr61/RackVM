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

#include "compiler.hpp"

namespace Compiler
{
    static std::string to_string(DataType dataType)
    {
        switch (dataType)
        {
            case DataType::INT:    return "int";
            case DataType::LONG:   return "long";
            case DataType::CHAR:   return "char";
            case DataType::FLOAT:  return "float";
            case DataType::DOUBLE: return "double";
            case DataType::STRING: return "string";
            case DataType::UNDEFINED: 
            default: return "void";
        }

        return "";
    }

    RackCompiler::RackCompiler() :
        m_traceParsing(false)
    {
    }

    RackCompiler::~RackCompiler()
    {
    }

    int RackCompiler::Parse(const std::string& file)
    {
        m_file = file;
        m_location.initialize(&m_file);

        std::ifstream inStream(file);

        RackLexer lexer(*this, &inStream);
        RackParser parser(lexer, *this);
        parser.set_debug_level(m_traceParsing);
        
        return parser.parse() == 0;
    }

    void RackCompiler::AddFunction(DataType dataType, std::string&& id, std::vector<stmt>&& statements)
    {
        func fun = {};
        fun.statements = std::move(statements);
        fun.identifier = std::move(id);
        fun.returnType = dataType;

        std::cout << "Added function \"" << to_string(fun.returnType) << " " << fun.identifier << "\":" << std::endl;

        auto it = fun.statements.begin();
        while (it != fun.statements.end())
            std::cout << "--- " << *it++ << std::endl;

        m_funcList.push_back(std::move(fun));
    }

    std::ostream& operator <<(std::ostream& os, const stmt& s)
    {
        switch (s.type)
        {
            case stmt_type::ASSIGNMENT: os << "assign: " << s.identifier << 
                " = expr{" << s.expression << "}"; 
                break;

            case stmt_type::DECLARATION: os << "decl: " << s.dataType << " " << s.identifier; 
                break;

            case stmt_type::INITIALIZATION: os << "init: " << s.dataType << " " << s.identifier << 
                " = expr{" << s.expression << "}"; 
                break;

            case stmt_type::EXPRESSION: os << "expr: " << s.expression; 
                break;

            default: os << "Other"; break;
        }

        return os;
    }

    std::ostream& operator <<(std::ostream& os, const DataType& dataType)
    {
        switch (dataType)
        {
            case DataType::INT:    os << "int";    break;
            case DataType::LONG:   os << "long";   break;
            case DataType::CHAR:   os << "char";   break;
            case DataType::FLOAT:  os << "float";  break;
            case DataType::DOUBLE: os << "double"; break;
            case DataType::STRING: os << "string"; break;
            case DataType::UNDEFINED: 
            default: os << "void";
        }

        return os;
    }

    std::ostream& operator <<(std::ostream& os, const expr_type& e)
    {
        switch (e)
        {
            case expr_type::ID:     os << "id";     break;
            case expr_type::NUMBER: os << "number"; break;
            case expr_type::ADD:    os << "add";    break;
            case expr_type::SUB:    os << "sub";    break;
            case expr_type::MUL:    os << "mul";    break;
            case expr_type::DIV:    os << "div";    break;
            case expr_type::EQ:     os << "eq";     break;
            case expr_type::NEQ:    os << "neq";    break;
            case expr_type::GT:     os << "gt";     break;
            case expr_type::LT:     os << "lt";     break;
            case expr_type::GEQ:    os << "geq";    break;
            case expr_type::LEQ:    os << "leq";    break;
            case expr_type::OR:     os << "or";     break;
            case expr_type::AND:    os << "and";    break;
            case expr_type::NEG:    os << "or";     break;
            case expr_type::CALL:   os << "call";   break;

            default: os << "unknown expr_type";     break;
        }

        return os;
    }

    std::ostream& operator <<(std::ostream& os, const expr& e)
    {
        switch (e.type)
        {
            case expr_type::ID:     os << e.strValue;     break;
            case expr_type::NUMBER: os << (e.dataType == DataType::INT ? e.intValue : e.longValue);        break;
            case expr_type::ADD:    os << "(" << e.operands.front() << " + " << e.operands.back() << ")";  break;
            case expr_type::SUB:    os << "(" << e.operands.front() << " - " << e.operands.back() << ")";  break;
            case expr_type::MUL:    os << "(" << e.operands.front() << " * " << e.operands.back() << ")";  break;
            case expr_type::DIV:    os << "(" << e.operands.front() << " / " << e.operands.back() << ")";  break;
            case expr_type::EQ:     os << "(" << e.operands.front() << " == " << e.operands.back() << ")"; break;
            case expr_type::NEQ:    os << "(" << e.operands.front() << " != " << e.operands.back() << ")"; break;
            case expr_type::GT:     os << "(" << e.operands.front() << " > " << e.operands.back() << ")";  break;
            case expr_type::LT:     os << "(" << e.operands.front() << " < " << e.operands.back() << ")";  break;
            case expr_type::GEQ:    os << "(" << e.operands.front() << " >= " << e.operands.back() << ")"; break;
            case expr_type::LEQ:    os << "(" << e.operands.front() << " <= " << e.operands.back() << ")"; break;
            case expr_type::OR:     os << "(" << e.operands.front() << " || " << e.operands.back() << ")"; break;
            case expr_type::AND:    os << "(" << e.operands.front() << " && " << e.operands.back() << ")"; break;
            case expr_type::NEG:    os << "(-(" << e.operands.front() << ")";     break;
            case expr_type::CALL:   os << e.operands.front().strValue << "()";   break;

            default: os << "unknown expr";     break;
        }

        return os;
    }
}

int main(int argc, char* argv[])
{
    Compiler::RackCompiler compiler;

    if (argc < 2)
    {
        std::cerr << "Invalid number of arguments" << std::endl;
        return EXIT_FAILURE;
    }

    if (argc >= 3)
    {
        if (std::strcmp(argv[2], "-t") == 0) // Check for -t flag which enables parser tracing.
        {
            compiler.m_traceParsing = true;
        }
    }

    if (!compiler.Parse(argv[1]))
    {
        std::cout << compiler.m_result << std::endl;
    }

    return 0;
}
