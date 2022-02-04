#include "assembler.hpp"

namespace Assembly {

    Assembler::Assembler() :
        m_hasError(false),
        m_lineNbr(0),
        m_instrNbr(0)
    {
    }

    std::string Assembler::RemoveWhitespace(const std::string& str) const
    {   
        std::string out;
        char ch;
        auto it = str.cbegin();
        while(it != str.cend())
        {
            ch = *it++;
            if (!::isspace(ch))
                out += ch;
        }

        return out;
    }

    void Assembler::ExecAssemblerDirective(const std::string& directive, std::string args[3])
    {
        // Make args[0] lower-case.
        std::transform(args[0].begin(), args[0].end(), args[0].begin(),
            [](unsigned char ch) { return ::tolower(ch); });

        if (directive == ".MODE") // Sets header info field 'MODE'.
        {
            if (args[0] != "register" && args[0] != "stack")
            {
                std::cerr << "Invalid argument for directive \".MODE\" at line " << m_lineNbr << std::endl;
                m_hasError = true;
                return;
            }

            m_binHeader.mode = args[0] == "register" ? VM_MODE_REGISTER : VM_MODE_STACK;
        }
        else if (directive == ".HEAP") // Sets header info field 'HEAP'.
        {
            if (args[0].find_first_not_of("0123456789") != std::string::npos)
            {
                std::cerr << "Invalid argument for directive \".HEAP\" at line " << m_lineNbr
                    << ": must be an unsigned 32-bit integer." << std::endl;
                m_hasError = true;
                return;
            }

            m_binHeader.heap = std::stoul(args[0]);
        }
        else if (directive == ".HEAP_MAX") // Sets header info field 'HEAP_MAX'.
        {
            if (args[0].find_first_not_of("0123456789") != std::string::npos)
            {
                std::cerr << "Invalid argument for directive \".HEAP_MAX\" at line " << m_lineNbr
                    << ": must be an unsigned 32-bit integer." << std::endl;
                m_hasError = true;
                return;
            }

            m_binHeader.heap_max = std::stoul(args[0]);
        }
        else
        {
            std::cerr << "Unknown assembler directive \"" << directive << "\" at line " 
                << m_lineNbr << "." << std::endl;
        }
    }

    void Assembler::FirstPassReadLine(std::string& line)
    {
        constexpr char whitespace[] = { ' ', '\t', '\0' };
        constexpr char wsOrComment[] = { ' ', '\t', ';', '/', '\0' };
        constexpr char wsOrCommentOrComma[] = { ' ', '\t', ';', '/', ',', '\0' };

        size_t len = line.length();
        size_t pos = line.find_first_not_of(whitespace);
        if (pos == std::string::npos)
            return;

        char ch = line[pos];

        // Discard lines that starts with comments.
        bool isSemicolonComment = ch == ';';
        bool isSlashComment = pos + 1 < len && ch == '/' && line[pos + 1] == '/';
        if (isSemicolonComment || isSlashComment)
            return;

        size_t posDelim = line.find(':', pos); // Look for label delimiter.
        if (posDelim != std::string::npos)
        {
            std::string label = line.substr(pos, posDelim);
            if (!m_labelDict.RegisterLabel(label, m_instrNbr))
            {
                std::cerr << "Multiple label definitions: \"" << label << "\" at line " << m_lineNbr << "." << std::endl;
                m_hasError = true;
            }
            return;
        }

        std::string opcode = "";
        std::string args[3] = {"", "", ""};

        // Try to read the opcode.
        posDelim = std::min(line.find_first_of(wsOrComment, pos), len); // Cap to string length if not found.
        opcode = line.substr(pos, posDelim - pos);

        // Try to read arguments (max 3).
        for (int i = 0; i < 3; i++)
        {
            posDelim += line[posDelim] == ',' ? 1 : 0;
            pos = line.find_first_not_of(whitespace, posDelim);
            if (pos != std::string::npos)
            {
                posDelim = std::min(line.find(",", pos), len); // Cap to string length if not found.
                size_t posComment = std::min(line.find_first_of(";/", pos), len); // Cap to string length if not found.
                if (posDelim != len && posDelim < posComment)
                {
                    args[i] = RemoveWhitespace(line.substr(pos, posDelim - pos));
                }
                else if (posDelim == len)
                {   
                    posDelim = posComment;

                    // Check single '/' for division, in that case find comment.
                    while (posDelim + 1 < len && line[posDelim] == '/' && line[posDelim + 1] != '/')
                    {
                        posDelim = std::min(line.find_first_of(";/", posDelim + 1), len);
                    }

                    args[i] = RemoveWhitespace(line.substr(pos, posDelim - pos));
                }
            }
        }

        // Output the opcode and arguments to workingText.
        if (opcode != "")
        {
            if (opcode[0] == '.')
            {
                ExecAssemblerDirective(opcode, args);
                return;
            }

            workingText << opcode;
        }
        else
        {
            std::cerr << "An instruction must start with a label or opcode:  at line " << m_lineNbr << "." << std::endl;
            m_hasError = true;
            return;
        }

        if (args[0] != "")
             workingText << " " << '"' << args[0] << '"';
        if (args[1] != "")
             workingText << ", " << '"' << args[1] << '"';
        if (args[2] != "")
             workingText << ", " << '"' << args[2] << '"';

        workingText << std::endl;
        m_instrNbr++;
    }

    Instruction Assembler::AssembleLine(std::string& line)
    {
        return 0x0;
    }

    size_t Assembler::Assemble(std::ifstream& textInput, const std::string& outPath, bool verbose)
    {
        m_hasError = false;

        std::ofstream outFile(outPath, std::ios::binary);
        if (!outFile.is_open())
        {
            std::cerr << "Could not create \"" << outPath << "\"." << std::endl;
            return 0;
        }

        // Temporary stream of assembly source that can be altered by the first pass,
        // and then read from by the second pass.
        workingText.clear();

        if (verbose)
            std::cout << "-------- FIRST PASS START --------" << std::endl;
        
        std::string line;
        for (m_lineNbr = 1, m_instrNbr = 0; std::getline(textInput, line); m_lineNbr++)
        {
            std::cout << m_lineNbr << " [" << m_instrNbr << "]\t" << line << std::endl;
            FirstPassReadLine(line);
        }

        if (verbose)
            std::cout << "-------- FIRST PASS END --------" << std::endl;

        outFile.write((const char*)&m_binHeader, sizeof(m_binHeader));

        if (verbose)
            std::cout << "-------- SECOND PASS BEGIN --------" << std::endl;

        for (m_instrNbr = 0; std::getline(workingText, line); m_instrNbr++)
        {
            Instruction binCode = AssembleLine(line);

            outFile << binCode;

            if (verbose)
                std::cout << "[" << m_instrNbr << "]\t" << line << "\t" << 
                    (binCode > 0 ? std::to_string(binCode) : "") << std::endl;
        }

        if (verbose)
            std::cout << "-------- SECOND PASS END --------" << std::endl;

        if (verbose)
            m_labelDict.WarnAboutUnusedLabels();

        if (m_hasError)
        {
            outFile.close();
            std::remove(outPath.c_str());
            return 0;
        }

        outFile.flush();
        return m_instrNbr;
    }
}

using namespace Assembly;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Invalid arguments." << std::endl;
        return 0;
    }

    std::string inputPath = argv[1]; 
    std::ifstream inputFile(inputPath);

    if (!inputFile.is_open())
    {
        std::cerr << "Could not open \"" << inputPath << "\"." << std::endl;
        return 0;
    }

    size_t pathLastDot = inputPath.rfind('.');
    if (pathLastDot == std::string::npos)
        pathLastDot = inputPath.length();

    std::string outputPath = inputPath.substr(0, pathLastDot) + ".bin";

    Assembler assembler;
    size_t binarySize = assembler.Assemble(inputFile, outputPath, true);
    if (binarySize > 0)
    {
        std::cout << "Assembly successful! Instructions total: " << binarySize << "." << std::endl;
    }
    else
    {
        std::cout << "Assembly failed!" << std::endl;
    }

    return 0;
}

