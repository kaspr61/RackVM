#include "compiler.hpp"

namespace Compiler
{
    RackCompiler::RackCompiler() :
        m_traceParsing(false)
    {
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
        RackParser parser(lexer, *this);
        parser.set_debug_level(m_traceParsing);
        
        return parser.parse() == 0;
    }
}

int main(int argc, char* argv[])
{
    Compiler::RackCompiler compiler;

    if (argc < 2)
    {
        std::cerr << "Invalid number of arguments" << std::endl;
        return EXIT_FAILURE;
    }

    if (argc >= 3)
    {
        if (std::strcmp(argv[2], "-t") == 0) // Check for -t flag which enables parser tracing.
        {
            compiler.m_traceParsing = true;
        }
    }

    if (!compiler.Parse(argv[1]))
    {
        std::cout << compiler.m_result << std::endl;
    }

    return 0;
}
