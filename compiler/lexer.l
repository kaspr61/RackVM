%{ /* -*- C++ -*- */
    #include <cerrno>
    #include <climits>
    #include <cstdlib>
    #include <cstring> // strerror
    #include <string>
    #include "compiler.hpp"
    #include "parser.hpp"
    #include "lexer.hpp"
%}

%option noyywrap nounput noinput batch debug
%option c++
%option yyclass="RackLexer"
%option prefix="Compiler_"

%{
    // A number symbol corresponding to the value in S.
    Compiler::RackParser::symbol_type
    make_NUM (const std::string &s, const Compiler::RackParser::location_type& loc);
%}

id          [a-zA-Z][a-zA-Z_0-9]*
num         [0-9]+
blank       [ \t\r]

%{
    // Code run each time a pattern is matched.
    # define YY_USER_ACTION  loc.columns (yyleng);
%}

%%

%{
    // A handy shortcut to the location held by the driver.
    Compiler::location& loc = cmp.m_location;
    // Code run each time yylex is called.
    loc.step ();
%}

{blank}+    loc.step ();
\n+         loc.lines (yyleng); loc.step ();

"="         return Compiler::RackParser::make_ASSIGN(loc);
"+"         return Compiler::RackParser::make_PLUS(loc);
"-"         return Compiler::RackParser::make_MINUS(loc);
"*"         return Compiler::RackParser::make_MULT(loc);
"/"         return Compiler::RackParser::make_DIV(loc);
"("         return Compiler::RackParser::make_L_PAR(loc);
")"         return Compiler::RackParser::make_R_PAR(loc);

{id}        return Compiler::RackParser::make_ID(yytext, loc);
{num}       return make_NUM(yytext, loc);

.          {
             throw Compiler::RackParser::syntax_error
               (loc, "invalid character: " + std::string(yytext));
}
<<EOF>>    return Compiler::RackParser::make_YYEOF (loc);

%%

Compiler::RackParser::symbol_type
make_NUM (const std::string &s, const Compiler::RackParser::location_type& loc)
{
    errno = 0;
    long long n = strtoll (s.c_str(), NULL, 10);
    if (! (LLONG_MIN <= n && n <= LLONG_MAX && errno != ERANGE))
        throw Compiler::RackParser::syntax_error (loc, "integer is out of range: " + s);

    return Compiler::RackParser::make_NUM ((long long) n, loc);
}
