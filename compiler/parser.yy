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
    #define TypeCheck(ex) {std::string err = ex.CheckType(); if (err != "") error(cmp.m_location, err);}
    #define TypeCheckAssign(st) {std::string err = st.CheckAssignmentType(); if (err != "") error(cmp.m_location, err);}
    #define TypeCheckReturn(st) {std::string err = cmp.CheckReturnType(st); if (err != "") error(cmp.m_location, err);}
    #define CheckArrayCreation(st) {std::string err = cmp.CheckArrayCreation(st); if (err != "") error(cmp.m_location, err);}
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
    NEQ       "not equals"
    OR        "or"
    AND       "and"
    GT        ">"
    LT        "<"
    GEQ       ">="
    LEQ       "<="
    L_CURL    "{"
    R_CURL    "}"
    L_SQ      "["
    R_SQ      "]"
    SEMICOLON ";"
    COMMA     ","
    STREQ     "starts with"
    ;

 /* Keywords */
%token
    IF      "if"
    ELSE    "else"
    WHILE   "while"
    INT     "int"
    LONG    "long"
    CHAR    "char"
    FLOAT   "float"
    DOUBLE  "double"
    STRING  "string"
    CREATE  "create"
    DESTROY "destroy"
    RETURN  "return"

 /* Dynamic tokens */
%token<std::string> ID          "id"
%token<int32_t>     NUM_INT     "int literal"
%token<int64_t>     NUM_LONG    "long literal"
%token<std::string> STR         "string literal"

 /* Rules with types */
%type<func> func
%type<std::vector<stmt>> stmts args_comma func_args
%type<stmt> stmt cond_stmt arg_decl
%type<std::vector<expr>> expr_list_1 expr_list_0
%type<expr> expr number func_call binary_expr unary_expr
%type<DataType> data_type

%%

%nonassoc IFX;
%nonassoc ELSE;

%left  OR AND;
%left  GT LT GEQ LEQ STREQ;
%left  EQ NEQ;
%left  PLUS MINUS;
%left  MULT DIV;
%right NOT;

%start program;

program: funcs                  {}
       ;

data_type: INT                  {$$ = DataType::INT;}
         | LONG                 {$$ = DataType::LONG;}
         | FLOAT                {$$ = DataType::FLOAT;}
         | DOUBLE               {$$ = DataType::DOUBLE;}
         | STRING               {$$ = DataType::STRING;}
         | INT L_SQ R_SQ        {$$ = DataType::INT_ARR;}
         | LONG L_SQ R_SQ       {$$ = DataType::LONG_ARR;}
         | FLOAT L_SQ R_SQ      {$$ = DataType::FLOAT_ARR;}
         | DOUBLE L_SQ R_SQ     {$$ = DataType::DOUBLE_ARR;}
         | STRING L_SQ R_SQ     {$$ = DataType::STRING_ARR;}
         ;

number: NUM_INT                 {$$ = expr($1);}
      | NUM_LONG                {$$ = expr($1);}
      ;

arg_decl: data_type ID          {$$ = stmt(stmt_type::DECLARATION, cmp.DeclVar($1, M($2), identifier_type::ARG_VAR));}
        ;

args_comma: args_comma COMMA arg_decl   {$$ = M($1); $$.push_back(M($3));}
          | arg_decl                    {$$.push_back(M($1));}
          ;

func_args: args_comma           {$$ = M($1);}
         | %empty               {}
         ;

expr_list_1: expr_list_1 COMMA expr     {$$ = M($1); $$.push_back(M($3));}
           | expr                       {$$.push_back(M($1));}
           ;

expr_list_0: expr_list_1        {$$ = M($1);}
           | %empty             {}
           ;

funcs: funcs func               {cmp.ExitScope();}
     | func                     {cmp.ExitScope();}
     ;

func: {cmp.EnterScope();} ID L_PAR func_args R_PAR L_CURL {cmp.DeclFunc(DataType::UNDEFINED, M($2), M($4));} stmts R_CURL {cmp.AddFunc(M($8));}
    | {cmp.EnterScope();} data_type ID L_PAR func_args R_PAR L_CURL {cmp.DeclFunc($2, M($3), M($5));} stmts R_CURL        {cmp.AddFunc(M($9));}
    ;

stmts: stmts stmt               {$$ = M($1); $$.push_back(M($2));}
     | %empty                   {}
     ;

stmt: data_type ID SEMICOLON                    {$$ = stmt(stmt_type::DECLARATION, cmp.DeclVar($1, M($2), identifier_type::LOCAL_VAR));}
    | ID ASSIGN expr SEMICOLON                  {$$ = stmt(stmt_type::ASSIGNMENT, cmp.UseVar(M($1)), M($3)); TypeCheckAssign($$);}
    | ID ASSIGN CREATE expr SEMICOLON           {$$ = stmt(stmt_type::CREATION, cmp.UseVar(M($1)), M($4));}
    | ID L_SQ expr R_SQ ASSIGN expr SEMICOLON   {$$ = stmt(stmt_type::ASSIGN_OFFSET, cmp.UseVar(M($1)), {M($3), M($6)}); TypeCheckAssign($$);}
    | data_type ID ASSIGN expr SEMICOLON        {$$ = stmt(stmt_type::INITIALIZATION, cmp.DeclVar($1, M($2), identifier_type::LOCAL_VAR), M($4));  TypeCheckAssign($$);}
    | data_type ID ASSIGN CREATE expr SEMICOLON {$$ = stmt(stmt_type::CREATION, cmp.DeclVar($1, M($2), identifier_type::LOCAL_VAR), M($5)); CheckArrayCreation($$);}
 /* | DESTROY ID SEMICOLON                      {$$ = stmt(stmt_type::DESTRUCTION, cmp.UseVar(M($2)));} */
    | RETURN expr SEMICOLON                     {$$ = stmt(stmt_type::RETURN, M($2)); TypeCheckReturn($$);}
    | RETURN SEMICOLON                          {$$ = stmt(stmt_type::RETURN); TypeCheckReturn($$);}
    | func_call SEMICOLON                       {$$ = stmt(stmt_type::FUNC_CALL, M($1));}
    | cond_stmt                                 {$$ = M($1);}
    | {cmp.EnterScope();} L_CURL stmts R_CURL   {$$ = stmt(stmt_type::BLOCK, M($3)); cmp.ExitScope();}
    ;

cond_stmt: IF L_PAR expr R_PAR stmt %prec IFX {$$ = stmt(stmt_type::BRANCH, {stmt(stmt_type::BLOCK, M($3), {M($5)})});}
         | IF L_PAR expr R_PAR stmt ELSE stmt {$$ = stmt(stmt_type::BRANCH, {stmt(stmt_type::BLOCK, M($3), {M($5)}), stmt(stmt_type::BLOCK, {M($7)})});}
         ;

expr: ID                        {$$ = expr(cmp.UseVar(M($1)));}
    | number                    {$$ = M($1);}
    | STR                       {$$ = expr($1); cmp.AddStringLiteral($1);}
    | L_PAR expr R_PAR          {$$ = M($2);}
    | binary_expr               {$$ = M($1); TypeCheck($$);}
    | unary_expr                {$$ = M($1); TypeCheck($$);}
    | func_call                 {$$ = M($1); TypeCheck($$);}
    ;

binary_expr: expr PLUS expr     {$$ = expr(expr_type::ADD,   M($1), M($3));}
           | expr MINUS expr    {$$ = expr(expr_type::SUB,   M($1), M($3));}
           | expr MULT expr     {$$ = expr(expr_type::MUL,   M($1), M($3));}
           | expr DIV expr      {$$ = expr(expr_type::DIV,   M($1), M($3));}
           | expr EQ expr       {$$ = expr(expr_type::EQ,    M($1), M($3));}
           | expr NEQ expr      {$$ = expr(expr_type::NEQ,   M($1), M($3));}
           | expr GT expr       {$$ = expr(expr_type::GT,    M($1), M($3));}
           | expr LT expr       {$$ = expr(expr_type::LT,    M($1), M($3));}
           | expr GEQ expr      {$$ = expr(expr_type::GEQ,   M($1), M($3));}
           | expr LEQ expr      {$$ = expr(expr_type::LEQ,   M($1), M($3));}
           | expr OR expr       {$$ = expr(expr_type::OR,    M($1), M($3));}
           | expr AND expr      {$$ = expr(expr_type::AND,   M($1), M($3));}
           | expr STREQ expr    {$$ = expr(expr_type::STREQ, M($1), M($3));}
           ;

unary_expr: MINUS expr                 {$$ = expr(expr_type::NEG, M($2));}
          | NOT expr                   {$$ = expr(expr_type::EQ,  M($2), (int32_t)0);}
          | ID L_SQ expr R_SQ          {$$ = expr(expr_type::ID_OFFSET, cmp.UseVar(M($1)), M($3));}
          | data_type L_PAR expr R_PAR {$$ = expr($1, M($3)); TypeCheck($$); }
          | data_type L_PAR expr COMMA expr R_PAR {$$ = expr($1, M($3), M($5)); TypeCheck($$); }
          ;

func_call: ID L_PAR expr_list_0 R_PAR   {$$ = expr(expr_type::CALL, cmp.UseFunc($1, $3), expr($3));}
         ;

%%

void Compiler::RackParser::error(const location_type& loc, const std::string& msg)
{
    std::cerr << loc << ": " << msg << '\n';
}
