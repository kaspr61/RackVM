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
    RackCompiler::RackCompiler() :
        m_traceParsing(false),
        m_currFunction()
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

    void RackCompiler::AddFunc(std::vector<stmt>&& statements)
    {
        m_currFunction.statements = std::move(statements);

        std::cout << "Added function \"" << m_currFunction.id << "\":" << std::endl;

        auto it = m_currFunction.statements.begin();
        while (it != m_currFunction.statements.end())
            std::cout << "--- " << *it++ << std::endl;

        m_funcList.push_back(std::move(m_currFunction));
        m_currFunction = {};
    }

    void RackCompiler::DeclFunc(DataType dataType, std::string&& id)
    {
        func& fun = m_currFunction;
        fun.id = identifier(identifier_type::FUNC_NAME, std::move(id), 0, dataType);
        fun.returnType = dataType;
    }

    const identifier& RackCompiler::DeclVar(DataType dataType, std::string&& varId, identifier_type idType)
    {
        if (m_scopes.empty())
            throw RackParser::syntax_error(m_location, "Variables may not be declared in global scope.");
        
        auto result = m_scopes.back().emplace(varId, 
            identifier(idType, varId, m_currFunction.localVarCnt++, dataType));
            
        if (!result.second)
            throw RackParser::syntax_error(m_location, "\""+varId+"\" has already been defined.");

        return result.first->second;
    }

    const identifier& RackCompiler::UseVar(std::string&& varId) const
    {
        for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
        {
            const ScopedVars& vars = *it;
            auto result = vars.find(varId);    
            if (result != vars.end())
                return (*result).second;

        }
        
        throw RackParser::syntax_error(m_location, "Unknown identifier \""+varId+"\".");
    }

    const func& RackCompiler::UseFunc(const std::string& funcId) const
    {
        const func* function;
        auto it = std::find_if(m_funcList.begin(), m_funcList.end(),
            [=](const func& e) { return e.id.id == funcId; });
         
        if (it != m_funcList.end())
            function = &*it;
        else
            function = &m_currFunction;

        if (function == nullptr || function->id.id != funcId)
            throw RackParser::syntax_error(m_location, "Unknown function \""+funcId+"\".");

        return *function;
    }

    std::ostream& operator <<(std::ostream& os, const stmt& s)
    {
        switch (s.type)
        {
            case stmt_type::ASSIGNMENT: os << "assign: (" << s.id.dataType << ") " << s.id << 
                " = expr{" << s.expression << "}"; 
                break;

            case stmt_type::DECLARATION: os << "decl: " << s.id.dataType << " " << s.id; 
                break;

            case stmt_type::INITIALIZATION: os << "init: " << s.id.dataType << " " << s.id << 
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

    std::ostream& operator <<(std::ostream& os, const identifier_type& id)
    {
        switch (id)
        {
            case identifier_type::LOCAL_VAR: os << "L";         break;
            case identifier_type::PARAM_VAR: os << "P";         break;
            case identifier_type::FUNC_NAME: os << "function";  break;

            default: os << "unknown identifier_type";     break;
        }

        return os;
    }

    std::ostream& operator <<(std::ostream& os, const identifier& id)
    {
        switch (id.type)
        {
            case identifier_type::LOCAL_VAR:
            case identifier_type::PARAM_VAR: os << id.id << "<" << id.type << id.position << ">"; break;
            case identifier_type::FUNC_NAME: os << id.id << "<" << id.dataType << ">()"; break;

            default: os << "unknown identifier \" " << id.id << "\"";     break;
        }

        return os;
    }

    std::ostream& operator <<(std::ostream& os, const expr& e)
    {
        switch (e.type)
        {
            case expr_type::ID:     os << e.id;                                                            break;
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
            case expr_type::NEG:    os << "(-(" << e.operands.front() << ")";                              break;
            case expr_type::CALL:   os << e.operands.front().id;                                           break;

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

    std::cout << compiler.Parse(argv[1]) << std::endl;

    return 0;
}
