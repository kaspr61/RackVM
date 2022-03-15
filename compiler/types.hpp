#ifndef INC_TYPES_HPP
#define INC_TYPES_HPP

#include <string>
#include <vector>
#include <list>
#include <ostream>

namespace Compiler
{
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

    struct expr
    {
        union {
            int32_t intValue;
            int64_t longValue;
        };
        
        std::string strValue;
     
        expr_type type;
        DataType dataType;
        std::list<expr> operands;

        expr() : dataType(DataType::UNDEFINED) {}

        template<typename... T>
        expr(expr_type type, T&&... args) : 
            type(type), 
            dataType(DataType::UNDEFINED), 
            operands{std::forward<T>(args)...} 
            {}

        expr(std::string&& value) : type(expr_type::ID), dataType(DataType::UNDEFINED), strValue(std::move(value)) {}
        expr(int32_t value) : type(expr_type::NUMBER), dataType(DataType::INT), intValue(value) {}
        expr(int64_t value) : type(expr_type::NUMBER), dataType(DataType::LONG), longValue(value) {}

        friend std::ostream& operator <<(std::ostream& os, const expr& s);
    };
    
    struct stmt
    {
        stmt_type   type;
        DataType    dataType;
        std::string identifier;
        expr        expression;

        stmt() :
            type(stmt_type::UNDEFINED),
            dataType(DataType::UNDEFINED),
            identifier(),
            expression()
        {
        }

        stmt(stmt_type type, expr&& expr) : 
            type(type),
            expression(std::move(expr))
        {
        }

        // For variable declaration.
        stmt(stmt_type type, DataType dataType, std::string&& id) :
            type(type),
            dataType(dataType), 
            identifier(std::move(id))
        {
        }

        // For variable assignment.
        stmt(stmt_type type, std::string&& id, expr&& expr) :
            type(type), 
            identifier(std::move(id)),
            expression(std::move(expr))
        {
        }

        // For variable declaration + initialization.
        stmt(stmt_type type, DataType dataType, std::string&& id, expr&& expr) :
            type(type),
            dataType(dataType), 
            identifier(std::move(id)),
            expression(std::move(expr))
        {
        }

        friend std::ostream& operator <<(std::ostream& os, const stmt& s);
    };

    struct func
    {
        DataType          returnType;
        std::string       identifier;
        std::vector<stmt> statements;
    };
}

#endif // INC_TYPES_HPP
