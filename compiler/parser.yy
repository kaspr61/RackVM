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

    namespace Compiler { class RackCompiler; class RackLexer; }

}

%parse-param { RackLexer& lex }
%parse-param { RackCompiler& cmp }
%lex-param   { RackLexer& lex }

%locations

%define parse.trace
%define parse.error detailed
%define parse.lac full

%code top {
    #include "compiler.hpp"
    #include "lexer.hpp"

    static Compiler::RackParser::symbol_type yylex(Compiler::RackLexer& lex)
    {
        return lex.get_next_token();
    }
}

%define api.token.prefix {T_}

 /* Static tokens */
%token
    ASSIGN  "="
    PLUS    "+"
    MINUS   "-"
    MULT    "*"
    DIV     "/"
    L_PAR   "("
    R_PAR   ")"
    ;

 /* Dynamic tokens */
%token<std::string> ID      "id"
%token<long long>   NUM     "number"

 /* Rules with types */

 /* Use << operator for printing. */
%printer { yyo << "VALUE: " << $$; } <*>;

%%

%start program;

program: {std::cout << "Program start..." << std::endl;} stmts {std::cout << "Program end..." << std::endl;};

stmts: stmts stmt               {}
     | %empty                   {}
     ;

stmt: assign                    {}
    ;

assign: ID ASSIGN expr          {std::cout << $1 << " was assigned." << std::endl;}
      ;

%left PLUS MINUS;
%left MULT DIV;
expr: ID                        {}
    | NUM                       {}
    | L_PAR expr R_PAR          {}
    | binary_expr               {}
    /* | unary_expr              {} */
    ;

binary_expr: expr PLUS expr     {}
           | expr MINUS expr    {}
           | expr MULT expr     {}
           | expr DIV expr      {}
           ;

%%

void Compiler::RackParser::error (const location_type& l, const std::string& m)
{
    std::cerr << l << ": " << m << '\n';
}
