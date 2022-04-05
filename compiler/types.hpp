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
#include <map>
#include <sstream>

#define ARRAY_TO_BASE(x) (::Compiler::g_arrToBaseType[x])

namespace Compiler
{
    struct func;
    struct stmt;

    enum class DataType : unsigned char
    {
        UNDEFINED,
        INT,
        LONG,
        FLOAT,
        DOUBLE,
        STRING,
        INT_ARR,
        LONG_ARR,
        FLOAT_ARR,
        DOUBLE_ARR,
        STRING_ARR
    };

    extern std::ostream& operator <<(std::ostream& os, const DataType& dataType);

    enum class expr_type
    {
        ID,
        ID_OFFSET,
        NUMBER,
        STRING,
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
        CALL,
        EXPR_LIST,
        CAST,
        STREQ       // Compares the first byte of two string.
    };

    extern std::ostream& operator <<(std::ostream& os, const expr_type& e);

    inline bool IsArray(DataType type)
    {
        return type == DataType::INT_ARR || type == DataType::LONG_ARR ||
               type == DataType::FLOAT_ARR || type == DataType::DOUBLE_ARR ||
               type == DataType::STRING_ARR;
    }

    // Returns the size of dataType in words (4 bytes each).
    extern int GetDataTypeBytes(DataType dataType);

    inline std::string Msg_ConflictingDataType(DataType lhs, DataType rhs)
    {
        return (std::stringstream() << "Conflicting data types: " << lhs << 
                " <--> " << rhs).str();
    }

    inline std::string Msg_ConflictingAssignDataType(DataType lhs, DataType rhs)
    {
        return (std::stringstream() << "Assigned as " << rhs << ", expects " << lhs << ".").str();
    }

    inline std::string Msg_IllegalExpression(DataType lhs, expr_type eType, DataType rhs)
    {
        return (std::stringstream() << "Illegal expression '" << eType << "' for types: " << lhs << 
                " <--> " << rhs).str();
    }

    inline std::string Msg_ArrayIndexNonInteger(DataType indexDataType)
    {
        return (std::stringstream() << "Array index must be an int: was " << indexDataType).str();
    }

    template<typename _Iterable>
    inline std::string BuildCommaListString(const _Iterable& list)
    {
        std::stringstream ss;
        typename _Iterable::const_iterator it = list.begin();
        while (true)
        {
            ss << *it++;
            if (it != list.end())
                ss << ", ";
            else
                break;
        }

        return ss.str();
    }

    static std::map<DataType, DataType> g_arrToBaseType = {
        std::make_pair(DataType::INT_ARR, DataType::INT),
        std::make_pair(DataType::LONG_ARR, DataType::LONG),
        std::make_pair(DataType::FLOAT_ARR, DataType::FLOAT),
        std::make_pair(DataType::DOUBLE_ARR, DataType::DOUBLE),
        std::make_pair(DataType::STRING_ARR, DataType::STRING),
        std::make_pair(DataType::STRING, DataType::STRING)
        };

    enum class stmt_type
    {
        UNDEFINED,
        DECLARATION,
        ASSIGNMENT,
        ASSIGN_OFFSET,
        INITIALIZATION,
        FUNC_CALL,
        BRANCH,     // Contains multiple 1 or more block statements.
        BLOCK,      // A block of statements, used for branching.
        CREATION,
        DESTRUCTION,
        RETURN
    };

    enum class identifier_type
    {
        UNDEFINED,
        LOCAL_VAR,
        ARG_VAR,
        FUNC_NAME,
        VARIADIC_ARG
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
            {
            }

        identifier(identifier_type type, const std::string& id, size_t position, DataType dataType) :
            type(type), id(id), position(position), dataType(dataType) 
            {
            }

        identifier(identifier_type type, DataType dataType) :
            type(type), id(), position(0), dataType(dataType) 
            {
            }

        friend std::ostream& operator <<(std::ostream& os, const identifier& s);
    };

    struct func
    {
        identifier               id;
        DataType                 returnType;
        size_t                   localVarCnt;
        size_t                   argVarCnt;
        std::vector<stmt>        statements;
        std::vector<stmt>        args;

        func() :
            id(),
            returnType(DataType::UNDEFINED),
            localVarCnt(0),
            argVarCnt(0),
            statements{},
            args{}
        {
        }

        func(const std::string& id, DataType retType, std::vector<stmt>&& args) :
            id(identifier_type::FUNC_NAME, id, 0, retType),
            returnType(retType),
            localVarCnt(0),
            argVarCnt(0),
            statements{},
            args(std::move(args))
        {
        }
    };

    struct expr
    {
        union {
            int32_t     intValue;
            int64_t     longValue;
            uint8_t     charValue;
            float       floatValue;
            double      doubleValue;
        };

        std::string strValue;
        
        identifier      id;
        expr_type       type;
        DataType        dataType;
        const func*     function;
        std::vector<expr> operands;

        expr() : dataType(DataType::UNDEFINED) {}

        template<typename... T>
        expr(expr_type type, T&&... args) : 
            type(type), 
            dataType(DataType::UNDEFINED), 
            operands{std::forward<T>(args)...} 
        {
        }

        expr(const std::vector<expr>& list) :
            type(expr_type::EXPR_LIST),
            dataType(DataType::UNDEFINED), 
            operands(list)
        {
        }

        expr(const identifier& value) : 
            type(expr_type::ID), 
            dataType(value.dataType), 
            id(value) 
        {
        }

        expr(identifier&& value) : 
            type(expr_type::ID), 
            dataType(value.dataType), 
            id(std::move(value)) 
        {
        }

        expr(int32_t value) : 
            type(expr_type::NUMBER), 
            dataType(DataType::INT), 
            intValue(value) 
        {
        }
        
        expr(int64_t value) : 
            type(expr_type::NUMBER), 
            dataType(DataType::LONG), 
            longValue(value) 
        {
        }

        expr(std::string&& value) : 
            type(expr_type::STRING), 
            dataType(DataType::STRING), 
            strValue(std::move(value)) 
        {
        }

        expr(const std::string& value) : 
            type(expr_type::STRING), 
            dataType(DataType::STRING), 
            strValue(value) 
        {
        }

        expr(const func& value) : 
            type(expr_type::ID), 
            dataType(value.returnType), 
            id(value.id), 
            function(&value) 
        {
        }

        expr(DataType castType, expr&& value) : 
            type(expr_type::CAST), 
            dataType(castType), 
            operands({std::move(value)})
        {
        }

        expr(DataType castType, expr&& value, expr&& castArg) : 
            type(expr_type::CAST), 
            dataType(castType), 
            operands({std::move(value), std::move(castArg)})
        {
        }

        static bool TryTypeCast(expr& e, DataType toType)
        {
            expr& castExpr = e;
            DataType fromType = e.dataType;

            // Integer promotion.
            if (fromType == DataType::INT && toType == DataType::LONG)
            {
                if (e.type == expr_type::NUMBER) // If e is a literal.
                {
                    e.dataType = DataType::LONG;
                    e.longValue = static_cast<int64_t>(e.intValue);
                    return true;
                }
                else // Not a literal, must do explicit cast expression.
                {
                    e = expr(toType, std::move(e));
                }
            }
            else if (fromType == DataType::FLOAT && toType == DataType::DOUBLE)
            {
                if (e.type == expr_type::NUMBER) // If e is a literal.
                {
                    e.dataType = DataType::DOUBLE;
                    e.doubleValue = static_cast<double>(e.floatValue);
                    return true;
                }
                else // Not a literal, must do explicit cast expression.
                {
                    e = expr(toType, std::move(e));
                }
            }
            else
            {
                return false;
            }

            return true;
        }

        bool TryTypePromotion(expr& lhs, expr& rhs)
        {
            DataType ltype = lhs.dataType;
            DataType rtype = rhs.dataType;
            if (ltype == DataType::INT && rtype == DataType::LONG)
            {
                // Promote lhs to long.
                return TryTypeCast(lhs, DataType::LONG);
            }
            else if (ltype == DataType::LONG && rtype == DataType::INT)
            {
                // Promote rhs to long.
                return TryTypeCast(rhs, DataType::LONG);
            }

            return false;
        }

        std::string CheckType()
        {
            if (operands.size() < 1)
                return "";

            //---- Detect type for unary expression ----//

            expr& lhs = operands.front();
            if (dataType == DataType::UNDEFINED)
                dataType = lhs.dataType;

            // Continue if it has more than 1 operand.
            if (operands.size() < 2)
                return "";

            expr& rhs = operands.back();

            // Should not proceed if it's a function call.
            if (type == expr_type::CALL)
                return "";

            // Do special checks for the optional type cast parameter.
            if (type == expr_type::CAST)
            {
                return "";
            }

            // Special case check for comparing first byte of strings
            if (type == expr_type::STREQ)
            {
                if (lhs.dataType != DataType::STRING || rhs.dataType != DataType::STRING)
                    return "Expression \"starts with\" may only be used with strings.";

                return "";
            }

            // Special case check for array indexing.
            if (type == expr_type::ID_OFFSET)
            {
                dataType = ARRAY_TO_BASE(lhs.dataType);
                int elementSize = GetDataTypeBytes(dataType);

                if (rhs.dataType != DataType::INT)
                    return Msg_ArrayIndexNonInteger(rhs.dataType);
           
                // Preserve single-byte offsets for strings.
                // For non-strings, multiply the index by element byte size.
                if (lhs.dataType != DataType::STRING)
                {
                    // If the index is a literal, eaasyy
                    if (rhs.type == expr_type::NUMBER)
                        rhs.intValue *= elementSize;
                    else
                        rhs = expr(expr_type::MUL, expr((int32_t)elementSize), rhs);
                }

                return "";
            }

            //---- Detect type for binary expression ----//

            if (lhs.dataType != rhs.dataType)
            {
                // Always try to promote types (widen the narrow one) if they mismatch. If failed, give error message.
                if (!TryTypePromotion(lhs, rhs))
                    return Msg_ConflictingDataType(lhs.dataType, rhs.dataType);

                dataType = lhs.dataType; // lhs and rhs should now have the same data type.
            }

            // Check for illegal arithmetic expressions.
            if (lhs.dataType == DataType::STRING || rhs.dataType == DataType::STRING || 
                IsArray(lhs.dataType) || IsArray(rhs.dataType))
            {
                switch (type)
                {
                    case expr_type::ADD:
                    case expr_type::SUB:
                    case expr_type::MUL:
                    case expr_type::DIV:
                    case expr_type::GEQ:
                    case expr_type::GT:
                    case expr_type::LEQ:
                    case expr_type::LT:
                    case expr_type::AND:
                    case expr_type::OR: return Msg_IllegalExpression(lhs.dataType, type, rhs.dataType);
                }
            }

            return "";
        }

        friend std::ostream& operator <<(std::ostream& os, const expr& s);
    };
    
    struct stmt
    {
        stmt_type         type;
        identifier        id;
        std::vector<expr> expressions;
        std::vector<stmt> substmts;

        stmt() :
            type(stmt_type::UNDEFINED),
            id(),
            expressions(),
            substmts()
        {
        }

        stmt(stmt_type type) : 
            type(type),
            id(),
            expressions(),
            substmts()
        {
        }

        stmt(stmt_type type, expr&& expr) : 
            type(type),
            id(),
            expressions({std::move(expr)}),
            substmts()
        {
        }

        // For variable declaration.
        stmt(stmt_type type, const identifier& id) :
            type(type),
            id(id),
            expressions(),
            substmts()
        {
        }

        // For variable assignment / initialization.
        stmt(stmt_type type, const identifier& id, expr&& exp) :
            type(type), 
            id(id),
            expressions({std::move(exp)}),
            substmts()
        {
            CheckAssignmentType();
        }

        // For indexed variable assignment, eg. numbers[5] = ...
        stmt(stmt_type type, const identifier& id, std::vector<expr>&& exprs) :
            type(type), 
            id(id),
            expressions(std::move(exprs)),
            substmts()
        {
        }

        // For if-statements
        stmt(stmt_type type, expr&& cond_expr, std::vector<stmt>&& stmts) : 
            type(type),
            id(),
            expressions({std::move(cond_expr)}),
            substmts(stmts)
        {
        }

        // For block-statements.
        stmt(stmt_type type, std::vector<stmt>&& stmts) : 
            type(type),
            id(),
            expressions(),
            substmts(stmts)
        {
        }

        std::string CheckAssignmentType()
        {
            if (type == stmt_type::ASSIGNMENT || type == stmt_type::INITIALIZATION)
            {
                expr& value = expressions.front();
                DataType valueDataType = value.dataType;
                if (value.operands.size() > 0 && value.type != expr_type::ID_OFFSET && value.type != expr_type::CAST)
                    if (value.operands.front().type == expr_type::ID)
                        valueDataType = value.operands.front().dataType;

                if (valueDataType != id.dataType)
                {
                    if (!expr::TryTypeCast(value, id.dataType))
                        return Msg_ConflictingAssignDataType(id.dataType, valueDataType);
                }
            }
            else if (type == stmt_type::ASSIGN_OFFSET)
            {
                const expr& index = expressions.front();
                expr& value = expressions.back();
                DataType arrType = ARRAY_TO_BASE(id.dataType);

                if (index.dataType != DataType::INT && index.dataType != DataType::LONG)
                    return Msg_ArrayIndexNonInteger(index.dataType);

                if (value.dataType != arrType)
                {
                    if (!expr::TryTypeCast(value, id.dataType))
                        return Msg_ConflictingAssignDataType(arrType, value.dataType);
                }
            }

            return "";
        }

        friend std::ostream& operator <<(std::ostream& os, const stmt& s);
    };
}

#endif // INC_TYPES_HPP
