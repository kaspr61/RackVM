#include "assembler.hpp"

#define LineError(msg) if (!(m_flags & FLAG_SUPPRESS_ALL_ERRORS)){\
    std::cerr << "[Assembler]: Error at line " << m_lineNbr << ": " << msg \
    << std::endl; m_hasError = true; m_lastError = m_lineNbr;}
#define InstructionError(msg) if (!(m_flags & FLAG_SUPPRESS_ALL_ERRORS)){\
    std::cerr << "[Assembler]: Error at instruction " << m_instrNbr << ": " \
    << msg << std::endl; m_hasError = true; m_lastError = m_instrNbr;}

namespace Assembly {

    Assembler::Assembler() :
        m_hasError(false),
        m_lineNbr(0),
        m_instrNbr(0),
        m_flags(0x0)
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
            if (!std::isspace(ch))
                out += ch;
        }

        return out;
    }

    uint32_t Assembler::EvaluateArgument(const std::string& arg)
    {
        size_t pos;
        uint32_t result = 0;

        // Check is numeric unary expression.
        if (arg.find_first_not_of("-0123456789") == std::string::npos) // Only contains digits and/or '-'.
        {
            pos = arg.rfind('-');
            if (pos == std::string::npos || pos == 0) // Is positive or negative integer
            {
                return std::stoul(arg.c_str());
            }
        }
        
        // Check is binary expression (including expressions with labels).
        if ((pos = arg.find_first_of("+-*/")) != std::string::npos)
        {
            if (arg.find_last_of("+-*/") != pos)
            {
                InstructionError("Invalid argument \"" << arg << "\": expressions with multiple "\
                    << "operators are not supported.");
                return result;
            }

            std::string leftStr = arg.substr(0, pos);
            std::string rightStr = arg.substr(pos + 1);
            char op = arg[pos];

            uint32_t left, right;
            if (leftStr.find_first_not_of("0123456789") == std::string::npos)
            {
                left = std::stoul(leftStr);
            }
            else if (!m_labelDict.ResolveLabel(leftStr, left))
            {
                InstructionError("Use of undefined label \"" << leftStr << "\".");
                return 0;
            }

            if (rightStr.find_first_not_of("0123456789") == std::string::npos)
            {
                right = std::stoul(rightStr);
            }
            else if (!m_labelDict.ResolveLabel(rightStr, right))
            {
                InstructionError("Use of undefined label \"" << rightStr << "\".");
                return 0;
            }

            switch (op)
            {
                case '+': result = left + right; break;
                case '-': result = left - right; break;
                case '*': result = left * right; break;
                case '/': result = left / right; break;
            }

            return result;
        }
        else // Treat as a label
        {
            int isNegative = arg[0] == '-' ? 1 : 0;
            std::string label = arg.substr(isNegative);

            if (!m_labelDict.ResolveLabel(label, result))
            {
                InstructionError("Use of undefined label \"" << label << "\".");
                return 0;
            }

            if (isNegative)                
                result = -result;

            return result;
        }
        
        InstructionError("Invalid argument \"" << arg << "\".");
        return result;
    }

    void Assembler::ExecAssemblerDirective(const std::string& directive, std::string args[3])
    {
        // Make args[0] lower-case.
        std::transform(args[0].begin(), args[0].end(), args[0].begin(),
            [](unsigned char ch) { return ::tolower(ch); });

        if (directive == ".MODE") // Sets header info field 'MODE'.
        {
            if (m_instrNbr > 0)
            {
                LineError("Invalid use of directive \".MODE\". The VM mode may only be declared "\
                    "before instructions.");
                return;
            }

            if (args[0] != "register" && args[0] != "stack")
            {
                LineError("Invalid argument for directive \".MODE\"");
                return;
            }

            m_binHeader.mode = args[0] == "register" ? VM_MODE_REGISTER : VM_MODE_STACK;
        }
        else if (directive == ".HEAP") // Sets header info field 'HEAP'.
        {
            if (args[0].find_first_not_of("0123456789") != std::string::npos)
            {
                LineError("Invalid argument for directive \".HEAP\". Must be an unsigned "\
                    "32-bit integer.");
                return;
            }

            m_binHeader.heap = std::stoul(args[0]);
        }
        else if (directive == ".HEAP_MAX") // Sets header info field 'HEAP_MAX'.
        {
            if (args[0].find_first_not_of("0123456789") != std::string::npos)
            {
                LineError("Invalid argument for directive \".HEAP_MAX\". Must be an unsigned "\
                    "32-bit integer.");
                return;
            }

            m_binHeader.heap_max = std::stoul(args[0]);
        }
        else
        {
            LineError("Unknown assembler directive \"" << directive << "\".");
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

            // Check for invalid characters in the label.
            if (label.find_first_of(wsOrComment) != std::string::npos)
            {
                LineError("Labels may not contain white-space characters, ';', or '/': \""
                    << label << "\".");
                return;
            }

            // Try to register the label.
            if (!m_labelDict.RegisterLabel(label, m_instrNbr))
            {
                LineError("Multiple label definitions: \"" << label << "\".");
            }

            return;
        }

        std::string opcode = "";
        std::string args[3] = {"", "", ""};

        // Try to read the opcode.
        posDelim = std::min(line.find_first_of(wsOrComment, pos), len); // Cap to string length if not found.
        opcode = line.substr(pos, posDelim - pos);

        if (opcode.find_first_not_of("._ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos)
        {
            LineError("Invalid instruction \"" << opcode << "\".");
            return;
        }

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
            LineError("An instruction must start with a label or opcode");
            return;
        }

        if (args[0] != "")
             workingText << ";" << args[0];
        if (args[1] != "")
             workingText << ";" << args[1];
        if (args[2] != "")
             workingText << ";" << args[2];

        workingText << ";" << std::endl;
        m_instrNbr++;
    }

    Instruction Assembler::AssembleLine(std::string& line)
    {
        size_t start = 0;
        size_t end = 0;
        std::string opcode = "";
        std::string parsedArgs[3] = {"", "", ""};
        
        uint32_t args[3] = {0U, 0U, 0U};
        
        end = line.find(';');
        opcode = line.substr(start, end);
     
        for (int i = 0; i < 3; i++)
        {
            start = end + 1;
            end = line.find(';', start);
            if (end != std::string::npos)
            {
                parsedArgs[i] = line.substr(start, end - start);
                args[i] = EvaluateArgument(parsedArgs[i]);

                // TODO: check integer sizes of argument based on i and opcode.
            }
            else
                break;
        }

        std::stringstream finalTranslation;
        finalTranslation << opcode;
        
        if (parsedArgs[0] != "")
             finalTranslation << ";" << args[0];
        if (parsedArgs[1] != "")
             finalTranslation << ";" << args[1];
        if (parsedArgs[2] != "")
             finalTranslation << ";" << args[2];

        finalTranslation << ";";

        Instruction result = 0x0;

        // Do a horrible attempt at formatting into columns.
        if (m_flags & FLAG_SHOW_TRANSLATION)
        {
            std::string instrNbr = "[" + std::to_string(m_instrNbr) + "]";
            std::cout << std::setfill(' ') << std::left << std::setw(9) << instrNbr << std::setw(32) << line << 
                std::setw(18) << finalTranslation.str() << "0x" << std::setfill('0') << std::right << 
                std::setw(8) << std::hex << result << std::dec << std::endl;
        }

        return result;
    }

    size_t Assembler::Assemble(std::ifstream& textInput, const std::string& outPath)
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

        if (m_flags & FLAG_SHOW_FIRST_PASS)
            std::cout << "-------- FIRST PASS START --------" << std::endl;
        
        std::string line;
        for (m_lineNbr = 1, m_instrNbr = 0; std::getline(textInput, line); m_lineNbr++)
        {
            if (m_flags & FLAG_SHOW_FIRST_PASS)
                std::cout << m_lineNbr << " [" << m_instrNbr << "]\t" << line << std::endl;
        
            FirstPassReadLine(line);
        }

        if (m_flags & FLAG_SHOW_FIRST_PASS)
            std::cout << "-------- FIRST PASS END --------" << std::endl;

        outFile.write((const char*)&m_binHeader, sizeof(m_binHeader));

        if (m_flags & FLAG_SHOW_TRANSLATION)
            std::cout << "-------- SECOND PASS BEGIN --------" << std::endl;

        for (m_instrNbr = 0; std::getline(workingText, line); m_instrNbr++)
        {
            outFile << AssembleLine(line);
        }

        if (m_flags & FLAG_SHOW_TRANSLATION)
            std::cout << "-------- SECOND PASS END --------" << std::endl;

        if (!(m_flags & FLAG_SUPPRESS_UNUSED_LABELS))
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

    assembler.SetFlags(FLAG_VERBOSE);
    size_t binarySize = assembler.Assemble(inputFile, outputPath);

    if (binarySize > 0)
        std::cout << "Assembly successful! Instructions total: " << binarySize << "." << std::endl;
    else
        std::cout << "Assembly failed!" << std::endl;

    return 0;
}
