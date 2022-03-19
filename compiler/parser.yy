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

%skeleton "lalr1.cc" // -*- C++ -*-
%require "3.8.2"
%language "c++"
%header

%define api.namespace {Compiler}
%define api.parser.class {RackParser}
%define api.token.raw
%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    #include <string>
    #include <vector>
    #include "types.hpp"

    // Forward declare classes.
    namespace Compiler { class RackLexer; class RackCompiler; }
}

%parse-param { RackLexer& lex }
%parse-param { RackCompiler& cmp }
%lex-param   { RackLexer& lex }

%locations

%define parse.trace
%define parse.error detailed
%define parse.lac full

%code
{
    #include "compiler.hpp"
    #include "lexer.hpp"

    static Compiler::RackParser::symbol_type yylex(Compiler::RackLexer& lex)
    {
        return lex.get_next_token();
    }

    #define M(x) std::move(x)
}

%define api.token.prefix {T_}

 /* Static tokens */
%token
    ASSIGN    "="
    PLUS      "+"
    MINUS     "-"
    MULT      "*"
    DIV       "/"
    L_PAR     "("
    R_PAR     ")"
    NOT       "not"
    EQ        "equals"
    OR        "or"
    AND       "and"
    GT        ">"
    LT        "<"
    GEQ       ">="
    LEQ       "<="
    L_CURL    "{"
    R_CURL    "}"
    SEMICOLON ";"
    ;

 /* Keywords */
%token
    IF      "if"
    ELIF    "elif"
    ELSE    "else"
    WHILE   "while"
    INT     "int"
    LONG    "long"
    CHAR    "char"
    FLOAT   "float"
    DOUBLE  "double"
    STRING  "string"

 /* Dynamic tokens */
%token<std::string> ID          "id"
%token<int32_t>     NUM_INT     "int literal"
%token<int64_t>     NUM_LONG    "long literal"

 /* Rules with types */
%type<func> func
%type<std::vector<stmt>> stmts elif_stmts
%type<stmt> stmt cond_stmt if_stmt elif_stmt else_stmt
%type<expr> expr number func_call binary_expr unary_expr
%type<DataType> data_type

 /* Use << operator for printing. */
%printer { /*yyo << "VALUE: " << $$;*/ } <*>;

%%

%right NOT;
%left  PLUS MINUS;
%left  MULT DIV;
%left  GT LT GEQ LEQ;
%left  EQ NEQ;
%left  OR AND;

%start program;

program: funcs                  {}
       ;

data_type: INT                  {$$ = DataType::INT;}
         | LONG                 {$$ = DataType::LONG;}
         | CHAR                 {$$ = DataType::CHAR;}
         | FLOAT                {$$ = DataType::FLOAT;}
         | DOUBLE               {$$ = DataType::DOUBLE;}
         | STRING               {$$ = DataType::STRING;}
         ;

number: NUM_INT                {$$ = expr($1);}
      | NUM_LONG               {$$ = expr($1);}
      ;

funcs: funcs func               {cmp.ExitScope();}
     | func                     {cmp.ExitScope();}
     ;

func: {cmp.EnterScope();} ID L_PAR R_PAR L_CURL {cmp.DeclFunc(DataType::UNDEFINED, M($2));} stmts R_CURL {cmp.AddFunc(M($7));}
    | {cmp.EnterScope();} data_type ID L_PAR R_PAR L_CURL {cmp.DeclFunc($2, M($3));} stmts R_CURL        {cmp.AddFunc(M($8));}
    ;

stmts: stmts stmt               {$$ = M($1); $$.push_back(M($2));}
     | %empty                   {}
     ;

stmt: data_type ID SEMICOLON            {$$ = stmt(stmt_type::DECLARATION, cmp.DeclVar($1, M($2), identifier_type::LOCAL_VAR));}
    | ID ASSIGN expr SEMICOLON          {$$ = stmt(stmt_type::ASSIGNMENT, cmp.UseVar(M($1)), M($3));}
    | data_type ID ASSIGN expr SEMICOLON{$$ = stmt(stmt_type::INITIALIZATION, cmp.DeclVar($1, M($2), identifier_type::LOCAL_VAR), M($4));}
    | expr SEMICOLON                    {$$ = stmt(stmt_type::EXPRESSION, M($1));}
    | {cmp.EnterScope();} cond_stmt     {$$ = M($2); cmp.ExitScope();}
    ;

cond_stmt: if_stmt                                  {$$ = stmt(stmt_type::BRANCH, {M($1)});}
         | if_stmt else_stmt                        {$$ = stmt(stmt_type::BRANCH, {M($1), M($2)});}
         | if_stmt elif_stmts                       {$2.insert($2.begin(), M($1)); $$ = stmt(stmt_type::BRANCH, M($2));}
         | if_stmt elif_stmts else_stmt             {$2.insert($2.begin(), M($1)); $2.insert($2.end(), M($3)); $$ = stmt(stmt_type::BRANCH, M($2));}
         ;

if_stmt: IF L_PAR expr R_PAR L_CURL stmts R_CURL    {$$ = stmt(stmt_type::BLOCK, M($3), M($6));}
       | IF L_PAR expr R_PAR stmt                   {$$ = stmt(stmt_type::BLOCK, M($3), {M($5)});}
       ;

elif_stmts: elif_stmts elif_stmt                    {$$ = M($1); $$.push_back(M($2));}
          | elif_stmt                               {$$ = {M($1)};}
          ;

elif_stmt: ELIF L_PAR expr R_PAR L_CURL stmts R_CURL{$$ = stmt(stmt_type::BLOCK, M($3), M($6));}
         | ELIF L_PAR expr R_PAR stmt               {$$ = stmt(stmt_type::BLOCK, M($3), {M($5)});}
         ;

else_stmt: ELSE L_CURL stmts R_CURL                 {$$ = stmt(stmt_type::BLOCK, M($3));}
         | ELSE stmt                                {$$ = stmt(stmt_type::BLOCK, {M($2)});}
         ;

expr: ID                        {$$ = expr(cmp.UseVar(M($1)));}
    | number                    {$$ = M($1);}
    | L_PAR expr R_PAR          {$$ = M($2);}
    | binary_expr               {$$ = M($1);}
    | unary_expr                {$$ = M($1);}
    | func_call                 {$$ = M($1);}
    ;

binary_expr: expr PLUS expr     {$$ = expr(expr_type::ADD, M($1), M($3));}
           | expr MINUS expr    {$$ = expr(expr_type::SUB, M($1), M($3));}
           | expr MULT expr     {$$ = expr(expr_type::MUL, M($1), M($3));}
           | expr DIV expr      {$$ = expr(expr_type::DIV, M($1), M($3));}
           | expr EQ expr       {$$ = expr(expr_type::EQ,  M($1), M($3));}
           | expr NEQ expr      {$$ = expr(expr_type::NEQ, M($1), M($3));}
           | expr GT expr       {$$ = expr(expr_type::GT,  M($1), M($3));}
           | expr LT expr       {$$ = expr(expr_type::LT,  M($1), M($3));}
           | expr GEQ expr      {$$ = expr(expr_type::GEQ, M($1), M($3));}
           | expr LEQ expr      {$$ = expr(expr_type::LEQ, M($1), M($3));}
           | expr OR expr       {$$ = expr(expr_type::OR,  M($1), M($3));}
           | expr AND expr      {$$ = expr(expr_type::AND, M($1), M($3));}
           ;

unary_expr: MINUS expr          {$$ = expr(expr_type::NEG, M($2));}
          | NOT expr            {$$ = expr(expr_type::EQ,  M($2), 0);}
          ;

func_call: ID L_PAR R_PAR       {$$ = expr(expr_type::CALL, cmp.UseFunc($1));}
         ;

%%

void Compiler::RackParser::error(const location_type& loc, const std::string& msg)
{
    std::cerr << loc << ": " << msg << '\n';
}
