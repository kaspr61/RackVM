#ifndef INC_LEXER_HPP
#define INC_LEXER_HPP

#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer Compiler_FlexLexer
#include <FlexLexer.h>
#endif

#undef YY_DECL
#define YY_DECL Compiler::RackParser::symbol_type Compiler::RackLexer::get_next_token()

#include "parser.hpp"

namespace Compiler
{
    class RackCompiler;

    class RackLexer : public yyFlexLexer
    {
    private:
        RackCompiler &cmp;

    public:
        RackLexer(RackCompiler &cmp, std::istream *in) : yyFlexLexer(in), cmp(cmp) {}
        virtual ~RackLexer() {}

        virtual RackParser::symbol_type get_next_token();
    };
}

#endif // INC_LEXER_HPP
