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
    Compiler::RackParser::symbol_type
    make_NUM (const std::string &s, const Compiler::RackParser::location_type& loc);

    Compiler::RackParser::symbol_type
    make_STR_LIT (const std::string &s, const Compiler::RackParser::location_type& loc);

    Compiler::RackParser::symbol_type
    make_CHARACTER (const std::string &s, const Compiler::RackParser::location_type& loc);

    void multiln_comment_add_lines(char* str, Compiler::RackParser::location_type& loc);

%}

id          [a-zA-Z][a-zA-Z_0-9]*
num         [-]?[0-9]+
str         [\"][^\"\n]*[\"]
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

"//"[^\n]*           { /* Ignore single-line comment */ }
[/][*][^*]*[*]+([^*/][^*]*[*]+)*[/]  { /* Ignore multi-line comment */ multiln_comment_add_lines(yytext, loc); }
[/][*]               return Compiler::RackParser::make_YYEOF(loc);

{blank}+    loc.step();
\n+         loc.lines(yyleng); loc.step();

"="           return Compiler::RackParser::make_ASSIGN(loc);
"+"           return Compiler::RackParser::make_PLUS(loc);
"-"           return Compiler::RackParser::make_MINUS(loc);
"*"           return Compiler::RackParser::make_MULT(loc);
"/"           return Compiler::RackParser::make_DIV(loc);
"("           return Compiler::RackParser::make_L_PAR(loc);
")"           return Compiler::RackParser::make_R_PAR(loc);
"is not"      return Compiler::RackParser::make_NEQ(loc);       
"not"         return Compiler::RackParser::make_NOT(loc);       
"equals"      return Compiler::RackParser::make_EQ(loc);        
"is"          return Compiler::RackParser::make_EQ(loc);        
"or"          return Compiler::RackParser::make_OR(loc);       
"and"         return Compiler::RackParser::make_AND(loc);       
"is above"    return Compiler::RackParser::make_GT(loc);       
"is below"    return Compiler::RackParser::make_LT(loc);       
">"           return Compiler::RackParser::make_GT(loc);        
"<"           return Compiler::RackParser::make_LT(loc);        
">="          return Compiler::RackParser::make_GEQ(loc);       
"<="          return Compiler::RackParser::make_LEQ(loc);       
"{"           return Compiler::RackParser::make_L_CURL(loc);    
"}"           return Compiler::RackParser::make_R_CURL(loc);    
"["           return Compiler::RackParser::make_L_SQ(loc);    
"]"           return Compiler::RackParser::make_R_SQ(loc);    
";"           return Compiler::RackParser::make_SEMICOLON(loc);
","           return Compiler::RackParser::make_COMMA(loc);
"starts with" return Compiler::RackParser::make_STREQ(loc);

"if"          return Compiler::RackParser::make_IF(loc);
"else"        return Compiler::RackParser::make_ELSE(loc);
"while"       return Compiler::RackParser::make_WHILE(loc);
"int"         return Compiler::RackParser::make_INT(loc);
"long"        return Compiler::RackParser::make_LONG(loc);
"char"        return Compiler::RackParser::make_CHAR(loc);
"float"       return Compiler::RackParser::make_FLOAT(loc);
"double"      return Compiler::RackParser::make_DOUBLE(loc);
"string"      return Compiler::RackParser::make_STRING(loc);
"create"      return Compiler::RackParser::make_CREATE(loc);
"destroy"     return Compiler::RackParser::make_DESTROY(loc);
"return"      return Compiler::RackParser::make_RETURN(loc);

{id}          return Compiler::RackParser::make_ID(yytext, loc);
{num}         return make_NUM(yytext, loc);
{str}         return make_STR_LIT(yytext, loc);

.          {
            throw Compiler::RackParser::syntax_error(loc, 
                "invalid character: " + std::string(yytext));
}
<<EOF>>    return Compiler::RackParser::make_YYEOF (loc);

%%

Compiler::RackParser::symbol_type
make_NUM (const std::string &s, const Compiler::RackParser::location_type& loc)
{
    errno = 0;
    int64_t n = strtoll (s.c_str(), NULL, 10);
    if (n <= INT32_MAX && n >= INT32_MIN)
        return Compiler::RackParser::make_NUM_INT(static_cast<int32_t>(n), loc);
    else if (n <= INT64_MAX && n >= INT64_MIN)
        return Compiler::RackParser::make_NUM_LONG(static_cast<int64_t>(n), loc);

    throw Compiler::RackParser::syntax_error (loc, "Integer literal is out of range: " + s);
}

Compiler::RackParser::symbol_type
make_STR_LIT (const std::string &s, const Compiler::RackParser::location_type& loc)
{
    return Compiler::RackParser::make_STR(s.substr(1, s.size()-2), loc);
}

void multiln_comment_add_lines(char* str, Compiler::RackParser::location_type& loc)
{
    std::string s(str);
    auto count = std::count(s.begin(), s.end(), '\n');
    loc.lines(count);
}
