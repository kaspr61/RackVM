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

#ifndef INC_TYPES_HPP
#define INC_TYPES_HPP

#include <string>
#include <vector>
#include <list>
#include <ostream>

namespace Compiler
{
    struct func;
    struct stmt;

    enum class DataType
    {
        UNDEFINED,
        INT,
        LONG,
        CHAR,
        FLOAT,
        DOUBLE,
        STRING

    };

    extern std::ostream& operator <<(std::ostream& os, const DataType& dataType);

    enum class stmt_type
    {
        UNDEFINED,
        DECLARATION,
        ASSIGNMENT,
        INITIALIZATION,
        EXPRESSION,
        IF,
        IF_ELSE
    };

    enum class expr_type
    {
        ID,
        NUMBER,
        ADD,
        SUB,
        MUL,
        DIV,
        EQ, 
        NEQ,
        GT, 
        LT, 
        GEQ,
        LEQ,
        OR, 
        AND,
        NEG,
        CALL
    };

    extern std::ostream& operator <<(std::ostream& os, const expr_type& e);

    enum class identifier_type
    {
        UNDEFINED,
        LOCAL_VAR,
        PARAM_VAR,
        FUNC_NAME
    };

    extern std::ostream& operator <<(std::ostream& os, const identifier_type& e);

    struct identifier
    {
        identifier_type type;
        std::string     id;
        size_t          position;
        DataType        dataType;

        identifier() : 
            type(identifier_type::UNDEFINED), id(), position(0), dataType(DataType::UNDEFINED)
            {}

        identifier(identifier_type type, const std::string& id, size_t position, DataType dataType) :
            type(type), id(id), position(position), dataType(dataType) 
            {}

        friend std::ostream& operator <<(std::ostream& os, const identifier& s);
    };

    struct func
    {
        DataType           returnType;
        identifier         id;
        std::vector<stmt>  statements;
        size_t             localVarCnt;
    };

    struct expr
    {
        union {
            int32_t intValue;
            int64_t longValue;
        };
        
        identifier      id;
        expr_type       type;
        DataType        dataType;
        const func*     function;
        std::list<expr> operands;

        expr() : dataType(DataType::UNDEFINED) {}

        template<typename... T>
        expr(expr_type type, T&&... args) : 
            type(type), 
            dataType(DataType::UNDEFINED), 
            operands{std::forward<T>(args)...} 
            {}

        expr(const identifier& value) : type(expr_type::ID), dataType(DataType::UNDEFINED), id(value) {}
        expr(identifier&& value) : type(expr_type::ID), dataType(DataType::UNDEFINED), id(std::move(value)) {}
        expr(int32_t value) : type(expr_type::NUMBER), dataType(DataType::INT), intValue(value) {}
        expr(int64_t value) : type(expr_type::NUMBER), dataType(DataType::LONG), longValue(value) {}

        expr(const func& value) : 
            type(expr_type::ID), 
            dataType(DataType::UNDEFINED), 
            id(value.id), 
            function(&value) 
            {}

        friend std::ostream& operator <<(std::ostream& os, const expr& s);
    };
    
    struct stmt
    {
        stmt_type   type;
        identifier  id;
        expr        expression;

        stmt() :
            type(stmt_type::UNDEFINED),
            id(),
            expression()
        {
        }

        stmt(stmt_type type, expr&& expr) : 
            type(type)
        {
        }

        // For variable declaration.
        stmt(stmt_type type, const identifier& id) :
            type(type),
            id(id)
        {
        }

        // For variable assignment / initialization.
        stmt(stmt_type type, const identifier& id, expr&& expr) :
            type(type), 
            id(id),
            expression(std::move(expr))
        {
        }

        friend std::ostream& operator <<(std::ostream& os, const stmt& s);
    };
}

#endif // INC_TYPES_HPP
