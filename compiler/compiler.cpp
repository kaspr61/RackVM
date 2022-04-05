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
    ////======== COMPILER IMPLEMENTATION ========////

    RackCompiler::RackCompiler(CodeGenerationType codeGenType) :
        m_traceParsing(false),
        m_currFunction(),
        m_codeGenType(codeGenType),
        m_heapSize(0),
        m_maxHeapSize(0),
        m_funcList(),
        m_literals()
    {
#define SYSFUNC_ARG(dataType) stmt(stmt_type::DECLARATION, identifier(identifier_type::ARG_VAR, DataType::dataType))
#define SYSFUNC_VARIADIC_ARG() stmt(stmt_type::DECLARATION, identifier(identifier_type::VARIADIC_ARG, DataType::UNDEFINED))

        // Insert hardcoded system function declarations into the list of existing functions.
        m_funcList.insert(m_funcList.begin(), {
            func("__print", DataType::UNDEFINED, { SYSFUNC_ARG(STRING), SYSFUNC_VARIADIC_ARG() }),
            func("__input", DataType::STRING,    {}),
            func("__write", DataType::UNDEFINED, { SYSFUNC_ARG(INT), SYSFUNC_ARG(STRING) }),
            func("__read",  DataType::STRING,    { SYSFUNC_ARG(INT) }),
            func("__open",  DataType::INT,       { SYSFUNC_ARG(STRING), SYSFUNC_ARG(STRING) }),
            func("__close", DataType::UNDEFINED, { SYSFUNC_ARG(INT) }),
            func("__str",   DataType::STRING,    { SYSFUNC_ARG(STRING), SYSFUNC_VARIADIC_ARG() })
        });

#undef SYSFUNC_ARG
#undef SYSFUNC_VARIADIC_ARG
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

        m_parser = new RackParser(lexer, *this);
        m_parser->set_debug_level(m_traceParsing);
        
        int parseResult = m_parser->parse();

        delete m_parser;

        bool codeGenResult = true;
        if (m_codeGenType != CodeGenerationType::NONE)
        {
            CodeGenerator* codeGenerator;
            if (m_codeGenType == CodeGenerationType::STACK)
                codeGenerator = new StackCodeGenerator(m_heapSize, 
                                m_maxHeapSize, m_funcList, m_literals);
            else
                codeGenerator = new RegisterCodeGenerator(m_heapSize, 
                                m_maxHeapSize, m_funcList, m_literals);

            // Generate the assembly source.
            if (codeGenResult = codeGenerator->TranslateFunctions())
                codeGenerator->Flush(std::cout); // Flush to .asm file.
            else
                codeGenerator->Flush(std::cout); // Flush an error log.

            delete codeGenerator;
        }

        return parseResult && codeGenResult;
    }

    void RackCompiler::SetHeapSize(uint32_t initialSize, uint32_t maxSize)
    {
        m_heapSize = initialSize;
        m_maxHeapSize = maxSize;
    }

    void RackCompiler::AddFunc(std::vector<stmt>&& statements)
    {
        std::cout << "Added function \"" << m_currFunction.id << "\":" << std::endl;

        auto it = statements.begin();
        while (it != statements.end())
        {
            // Print the statement.
            std::cout << std::string(4, ' ') << *it << std::endl;
            it++;
        }

        const stmt& lastStmt = statements.back();
        // If function data type is void.
        if (m_currFunction.returnType == DataType::UNDEFINED)
        {
            if (statements.size() == 0 || lastStmt.type != stmt_type::RETURN)
                statements.push_back(stmt(stmt_type::RETURN));
        }
        else
        {
            if (statements.size() == 0 || lastStmt.type != stmt_type::RETURN)
                m_parser->error(m_location, "Function must return a value.");
        }

        m_currFunction.statements = std::move(statements);
        m_funcList.push_back(std::move(m_currFunction));
        m_currFunction = {};
    }

    void RackCompiler::DeclFunc(DataType dataType, std::string&& id, std::vector<stmt>&& args)
    {
        func& fun = m_currFunction;
        fun.id = identifier(identifier_type::FUNC_NAME, std::move(id), 0, dataType);
        fun.returnType = dataType;
        fun.args = std::move(args);
    }

    const identifier& RackCompiler::DeclVar(DataType dataType, std::string&& varId, identifier_type idType)
    {
        if (m_scopes.empty())
            throw RackParser::syntax_error(m_location, "Variables may not be declared in global scope.");
        
        size_t* varCount = &m_currFunction.localVarCnt;
        if (idType == identifier_type::ARG_VAR)
            varCount = &m_currFunction.argVarCnt;

        auto result = m_scopes.back().emplace(varId, 
            identifier(idType, varId, (*varCount)++, dataType));
            
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

    const func& RackCompiler::UseFunc(const std::string& funcId, const std::vector<expr>& args) const
    {
        const func* function;
        bool funcIdFound = false;
        std::string funcName = std::string(funcId);

        if (IsSystemFunction(funcId))
            funcName = "__" + funcId;

        auto it = std::find_if(m_funcList.begin(), m_funcList.end(),
            [&](const func& f) 
            {
                if (f.id.id != funcName)
                    return false;

                funcIdFound = true;
                return MatchFunctionArgs(f, args);
            });
         
        if (it != m_funcList.end())
            function = &*it;
        else
            function = &m_currFunction;

        if (function == nullptr || function->id.id != funcName)
        {
            if (!funcIdFound)
                throw RackParser::syntax_error(m_location, "Couldn't find function \""+funcId+"\".");
            else
                throw RackParser::syntax_error(m_location, "Function arguments did not match declaration for \""+funcId+"\".");
        }

        return *function;
    }

    void RackCompiler::AddStringLiteral(const std::string& literal)
    {
        auto it = m_literals.find(literal);
        
        // Not found? Create a new one.
        if (it == m_literals.end()) {
            m_literals.emplace(literal, "_S" + std::to_string(m_literals.size()));
        }
    }

    std::string RackCompiler::CheckReturnType(const stmt& retStmt) const
    {
        if (retStmt.expressions.size() == 0)
            return "";

        DataType valueRetType = retStmt.expressions.front().dataType;
        DataType funcRetType = m_currFunction.returnType;
        if (valueRetType != funcRetType)
            return (std::stringstream() << "Return value does not match function declaration: " << valueRetType << 
                    ", expects " << funcRetType).str();

        return "";
    }

    bool RackCompiler::MatchFunctionArgs(const func& fun, const std::vector<expr>& args) const
    {   
        if (fun.args.size() == 0 && args.size() == 0)
            return true;
        else if (args.size() == 0)
            return false;

        for (size_t i = 0; i < fun.args.size(); ++i)
        {
            const stmt& funArg = fun.args[i];

            // Accept 0..* args for a variadic function argument.
            if (funArg.id.type == identifier_type::VARIADIC_ARG)
                return true;

            if (i >= args.size())
                return false;
            
            const expr& givenArgs = args[i];

            if (funArg.id.dataType != givenArgs.dataType)
                return false;
        }

        if (args.size() > fun.args.size())
            return false;

        return true;
    }

    std::string RackCompiler::CheckArrayCreation(stmt& st)
    {
        // Check that array length expression is an int.
        if (st.expressions.back().dataType != DataType::INT)
            return (std::stringstream() << "Array length must be an int value: " << st.expressions.back().dataType).str();
        
        // Handle arrays of > 4 byte elements.
        int32_t dataTypeSize = GetDataTypeBytes(ARRAY_TO_BASE(st.id.dataType));
        expr arrayLen = std::move(st.expressions.back());
        st.expressions.pop_back();

        // If element data type is larger than 4 bytes, multiply array length by nbr of bytes.
        if (dataTypeSize > 4)
            st.expressions.emplace_back(expr_type::MUL, std::move(arrayLen), expr(dataTypeSize));

        st.expressions.back().CheckType();
        return "";
    }

    ////======== END OF COMPILER IMPLEMENTATION ========////

    std::ostream& operator <<(std::ostream& os, const stmt& s)
    {
        std::vector<stmt>::const_iterator it;        

        switch (s.type)
        {
            case stmt_type::ASSIGNMENT: os << "assign: (" << s.id.dataType << ") " << s.id << 
                " = expr{" << s.expressions.front() << "}"; 
                break;

            case stmt_type::ASSIGN_OFFSET: os << "assign: (" << s.id.dataType << ") " << s.id << 
                "[" << s.expressions.front() << "] = expr{" << s.expressions.back() << "}"; 
                break;

            case stmt_type::DECLARATION: os << "decl: " << s.id.dataType << " " << s.id; 
                break;

            case stmt_type::INITIALIZATION: os << "init: " << s.id.dataType << " " << s.id << 
                " = expr{" << s.expressions.front() << "}"; 
                break;

            case stmt_type::FUNC_CALL: os << "func_call: " << s.expressions.front(); 
                break;

            case stmt_type::CREATION: os << "create: " << s.expressions.front() << " " << ARRAY_TO_BASE(s.id.dataType) << "s at " << s.id; 
                break;

            case stmt_type::DESTRUCTION: os << "destroy: " << s.id; 
                break;

            case stmt_type::RETURN: 
                if (s.expressions.size() == 0)
                    os << "return"; 
                else
                    os << "return: expr{" << s.expressions.front() << "}"; 
                break;

            case stmt_type::BLOCK: 
                os << '{' << std::endl;
                it = s.substmts.begin();
                while (it != s.substmts.end())
                    os << std::string(8, ' ') << *it++ << std::endl;

                os << std::string(4, ' ') << '}';
                break;

            case stmt_type::BRANCH: 
                os << "if: ( " << s.substmts.front().expressions.front() << " )" << std::endl;
                
                it = s.substmts.front().substmts.begin();
                while (it != s.substmts.front().substmts.end())
                    os << std::string(4, ' ') << *it++ << std::endl;


                if (s.substmts.size() == 2) // Has an else statement
                {
                    os << std::string(4, ' ') << "else" << std::endl; 
                    it = s.substmts.back().substmts.begin();
                    while (it != s.substmts.back().substmts.end())
                        os << std::string(4, ' ') << *it++;
                }
                break;

            default: os << "unknown statement"; break;
        }

        return os;
    }

    std::ostream& operator <<(std::ostream& os, const DataType& dataType)
    {
        switch (dataType)
        {
            case DataType::INT:        os << "int";     break;
            case DataType::LONG:       os << "long";    break;
            case DataType::FLOAT:      os << "float";   break;
            case DataType::DOUBLE:     os << "double";  break;
            case DataType::STRING:     os << "string";  break;
            case DataType::INT_ARR:    os << "int[]";    break;
            case DataType::LONG_ARR:   os << "long[]";   break;
            case DataType::FLOAT_ARR:  os << "float[]";  break;
            case DataType::DOUBLE_ARR: os << "double[]"; break;
            case DataType::STRING_ARR: os << "string[]"; break;
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
            case expr_type::STREQ:  os << "starts with";   break;

            default: os << "unknown expr_type";     break;
        }

        return os;
    }

    std::ostream& operator <<(std::ostream& os, const identifier_type& id)
    {
        switch (id)
        {
            case identifier_type::LOCAL_VAR: os << "L";         break;
            case identifier_type::ARG_VAR:   os << "A";         break;
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
            case identifier_type::ARG_VAR: os << id.id << "<" << id.type << id.position << ">"; break;
            case identifier_type::FUNC_NAME: os << id.id << "<" << id.dataType << ">"; break;

            default: os << "unknown identifier \" " << id.id << "\"";     break;
        }

        return os;
    }

    std::ostream& operator <<(std::ostream& os, const expr& e)
    {
        switch (e.type)
        {
            case expr_type::ID:        os << e.id;                                                            break;
            case expr_type::ID_OFFSET: os << e.operands.front() << "[" << e.operands.back() << "]";                               break;
            case expr_type::NUMBER:    os << (e.dataType == DataType::INT ? e.intValue : e.longValue);        break;
            case expr_type::STRING:    os << '"' << e.strValue << '"';                                        break;
            case expr_type::ADD:       os << "(" << e.operands.front() << " + " << e.operands.back() << ")";  break;
            case expr_type::SUB:       os << "(" << e.operands.front() << " - " << e.operands.back() << ")";  break;
            case expr_type::MUL:       os << "(" << e.operands.front() << " * " << e.operands.back() << ")";  break;
            case expr_type::DIV:       os << "(" << e.operands.front() << " / " << e.operands.back() << ")";  break;
            case expr_type::EQ:        os << "(" << e.operands.front() << " == " << e.operands.back() << ")"; break;
            case expr_type::NEQ:       os << "(" << e.operands.front() << " != " << e.operands.back() << ")"; break;
            case expr_type::GT:        os << "(" << e.operands.front() << " > " << e.operands.back() << ")";  break;
            case expr_type::LT:        os << "(" << e.operands.front() << " < " << e.operands.back() << ")";  break;
            case expr_type::GEQ:       os << "(" << e.operands.front() << " >= " << e.operands.back() << ")"; break;
            case expr_type::LEQ:       os << "(" << e.operands.front() << " <= " << e.operands.back() << ")"; break;
            case expr_type::OR:        os << "(" << e.operands.front() << " || " << e.operands.back() << ")"; break;
            case expr_type::AND:       os << "(" << e.operands.front() << " && " << e.operands.back() << ")"; break;
            case expr_type::NEG:       os << "-(" << e.operands.front() << ")";                               break;
            case expr_type::CAST:      os << e.dataType << "(" << e.operands.front() << ")";                  break;
            case expr_type::STREQ:     os << "(" << e.operands.front() << " starts with " << e.operands.back() << ")"; break;
            case expr_type::CALL:      os << e.operands.front().id << '(' << (e.operands.back().operands.size() > 0 ?
                                             (BuildCommaListString(e.operands.back().operands)) : "") << ')'; break;

            default: os << "unknown expr";     break;
        }

        return os;
    }

    int GetDataTypeBytes(DataType dataType)
    {
        switch (dataType)
        {
            case DataType::INT:
            case DataType::FLOAT:
            case DataType::STRING:
            case DataType::INT_ARR:
            case DataType::LONG_ARR:
            case DataType::FLOAT_ARR:
            case DataType::DOUBLE_ARR:
            case DataType::STRING_ARR: return 4;
            case DataType::LONG:
            case DataType::DOUBLE:     return 8;
        }

        return 0;
    }
}

int main(int argc, char** argv)
{
#if defined(_WIN32) || defined(WIN32)
    SetConsoleOutputCP(CP_UTF8);
#endif

    if (argc > 8)
    {
        std::cerr << "Too many arguments." << std::endl;
        return EXIT_FAILURE;
    }

    using CLIArgs::ArgType;

    std::initializer_list<CLIArgs::ArgInfo> args = 
    {
        {"-r",          ArgType::NONE,  "Sets the code generation mode to 'register'."},
        {"-s",          ArgType::NONE,  "Sets the code generation mode to 'stack' (default)."},
        {"--heap",      ArgType::INT,   "Sets initial heap size of the compiled program."},
        {"--max-heap",  ArgType::INT,   "Sets maximum heap size of the compiled program."},
    };

    if (argc == 2 && (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0))
    {
        std::cout << "==== The Original Compiler for Rack and RackVM ====" << std::endl;
        for (auto it = args.begin(); it != args.end(); ++it)
            std::cout << "    " << std::left << std::setw(12) << it->id << it->description << std::endl;
        
        std::cout << std::endl;
        return EXIT_SUCCESS;
    }

    CLIArgs::ArgParser arg(std::move(args));
    if (!arg.Parse(argc, argv))
    {
        std::cerr << "Arguments could not be parsed correctly." << std::endl;
        return EXIT_FAILURE;
    }

    bool registerMode = arg.Get("-r", false);
    bool stackMode    = arg.Get("-s", true);
    int initHeapSize  = arg.Get("--heap", 0);
    int maxHeapSize   = arg.Get("--max-heap", 0);

    // File name should be the first argument without a leading '-'.
    int i;
    char* fileName;
    for (i = 1; i < argc; i++)
    {
        fileName = argv[i];
        if (*fileName != '-')
            break;
    }

    if (i >= argc)
    {
        std::cerr << "No file name specified." << std::endl;
        return EXIT_FAILURE;
    }

    auto codeType = Compiler::RackCompiler::CodeGenerationType::STACK;
    if (registerMode)
        codeType = Compiler::RackCompiler::CodeGenerationType::REGISTER;

    Compiler::RackCompiler compiler(codeType);

    compiler.SetHeapSize(initHeapSize, maxHeapSize);

    try
    {
        compiler.Parse(fileName);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
