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

#include "assembler.hpp"

#define LineError(msg) if (!(m_flags & FLAG_SUPPRESS_ALL_ERRORS)){\
    std::cerr << "[Assembler]: Error at line " << m_lineNbr << ": " << msg \
    << std::endl; m_hasError = true; m_lastError = m_lineNbr;}

#define InstructionError(msg) if (!(m_flags & FLAG_SUPPRESS_ALL_ERRORS)){\
    std::cerr << "[Assembler]: Error at address " << "0x" << std::hex << m_instrAddr << ": " \
    << msg << std::endl; m_hasError = true; m_lastError = m_instrAddr;}

namespace Assembly {

    Assembler::Assembler() :
        m_hasError(false),
        m_lineNbr(0),
        m_instrAddr(0),
        m_flags(0x0),
        m_encoder()
    {
    }

    std::string Assembler::ToLowerCase(const std::string& str) const
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char ch) { return ::tolower(ch); });

        return result;
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

    void Assembler::PrintMemory(void* address, size_t byteCount) const
    {
        uint8_t* ptr = (uint8_t*)address;
        for (size_t i = 0; i < byteCount; i++, ptr++)
            printf("%02hhX", *ptr);
    }

    BinaryInstruction Assembler::TranslateInstruction(const std::string& opcode, uint64_t* args)
    {
        BinaryInstruction result;
        result.instr[0] = 0;
        result.instr[1] = 0;
        result.instr[2] = 0;

        if (opcode.front() == '.')
            return result;

        return std::move(m_encoder.TranslateInstruction(opcode, args));
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

    void Assembler::ExecAssemblerDirective(const std::string& directive, const std::string* args)
    {
        // Make args[0] lower-case.
        std::string lowerArg0 = ToLowerCase(args[0]);

        if (directive == ".MODE") // Sets header info field 'MODE'.
        {
            if (m_instrAddr > 0)
            {
                LineError("Invalid use of directive \".MODE\". The VM mode may only be declared "\
                    "before instructions.");
                return;
            }

            if (lowerArg0 != "register" && lowerArg0 != "stack")
            {
                LineError("Invalid argument for directive \".MODE\"");
                return;
            }

            m_binHeader.mode = args[0] == "register" ? VM_MODE_REGISTER : VM_MODE_STACK;
            m_encoder.LoadInstructionSet(static_cast<VMMode>(m_binHeader.mode));
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
        else if (directive == ".BYTE") // Declares number of bytes of program data to be stored.
        {
            bool isString = args[1].find('"') != std::string::npos;

            if (args[0].find_first_not_of("0123456789") != std::string::npos)
            {
                LineError("Invalid size argument for directive \".BYTE\". Must be an unsigned integer.");
                return;
            }
            else if (args[1].find_first_not_of("\".f0123456789") != std::string::npos && !isString)
            {
                LineError("Invalid data argument for directive \".BYTE\". Must be an unsigned integer, float, double, or string.");
                return;
            }
            else if (args[1] == "")
            {
                LineError("No data defined for directive \".BYTE\".");
                return;
            }

            workingText << directive << ';' << args[0] << ';' << args[1] << ';' << std::endl;
            m_instrAddr += std::stoul(args[0]);
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
        if (pos == line.npos)
            return;

        char ch = line[pos];

        // Discard lines that starts with comments.
        bool isSemicolonComment = ch == ';';
        bool isSlashComment = pos + 1 < len && ch == '/' && line[pos + 1] == '/';
        if (isSemicolonComment || isSlashComment)
            return;

        size_t posDelim = line.find(':', pos); // Look for label delimiter.
        if (posDelim != line.npos)
        {
            std::string label = line.substr(pos, posDelim);

            // Check for invalid characters in the label.
            if (label.find_first_of(wsOrComment) != label.npos)
            {
                LineError("Labels may not contain white-space characters, ';', or '/': \""
                    << label << "\".");
                return;
            }

            // Try to register the label.
            if (!m_labelDict.RegisterLabel(label, m_instrAddr))
            {
                LineError("Multiple label definitions: \"" << label << "\".");
                return;
            }

            pos = line.find_first_not_of(whitespace, posDelim + 1);
            if (pos == line.npos)
                return;
        }

        std::string opcode = "";
        std::string args[3] = {"", "", ""};

        // Try to read the opcode.
        posDelim = std::min(line.find_first_of(wsOrComment, pos), len); // Cap to string length if not found.
        opcode = line.substr(pos, posDelim - pos);

        if (opcode.find_first_not_of("._ABCDEFGHIJKLMNOPQRSTUVWXYZ3264") != std::string::npos)
        {
            LineError("Invalid instruction \"" << opcode << "\".");
            return;
        }
        else if (opcode == "" || opcode[0] == ';' || opcode[0] == '/') // End if it's a comment.
        {
            return; 
        }

        size_t argsRequired = m_encoder.GetInstructionArgCount(opcode);
        if (opcode == ".BYTE")
            argsRequired = 2;

        // Try to read arguments (max 3).
        int argsGiven = 0;
        for (int i = 0; i < 3; i++)
        {
            posDelim += line[posDelim] == ',' ? 1 : 0;
            pos = line.find_first_not_of(whitespace, posDelim);
            if (pos != line.npos)
            {
                size_t posComment = std::min(line.find_first_of(";/", pos), len); // Cap to string length if not found.
                size_t posString = line.find('"', pos);
                posDelim = std::min(line.find(",", pos), len); // Cap to string length if not found.
                if (posDelim > posComment)
                    posDelim = len;

                if (posString != line.npos && posString < posComment && posString < posDelim)
                {
                    posDelim = line.find('"', posString + 1);
                    if (posDelim == line.npos)
                    {
                        LineError("Invalid argument. String has no ending \".");
                        break;
                    }

                    args[i] = line.substr(posString, (posDelim+1) - posString);
                    ++argsGiven;

                    posDelim = std::min(line.find(",", posDelim), len); // Cap to string length if not found.
                    continue;
                }

                if (posDelim != len && posDelim < posComment)
                {
                    args[i] = RemoveWhitespace(line.substr(pos, posDelim - pos));
                    ++argsGiven;
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
                    ++argsGiven;
                }
            }
        }

        if (argsGiven != argsRequired && opcode.front() != '.')
        {
            LineError("\"" << opcode << "\" was given " << argsGiven << 
                      " arguments, but expects " << argsRequired << ".");
            return;
        }

        size_t instrBytes = m_encoder.GetInstructionByteSize(opcode);

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
        m_instrAddr += instrBytes;
    }

    void Assembler::AssembleLine(std::string& line, std::iostream& binaryOutput)
    {
        size_t start = 0;
        size_t end = 0;
        std::string opcode = "";
        std::string parsedArgs[3] = {"", "", ""};
        
        uint64_t args[3] = {0U, 0U, 0U};
        
        end = line.find(';');
        opcode = line.substr(start, end);
     
        for (int i = 0; i < 3; i++)
        {
            start = end + 1;
            end = line.find(';', start);
            if (end != std::string::npos)
            {
                parsedArgs[i] = line.substr(start, end - start);
                
                if (opcode != ".BYTE")
                    args[i] = EvaluateArgument(parsedArgs[i]);

                // TODO: check integer size of argument based on i and opcode.
            }
            else
                break;
        }

        // Instruction size in bytes.
        uint64_t sizeArg = 0;
        if (opcode == ".BYTE")
            sizeArg = std::stoull(parsedArgs[0]);

        size_t instrBytes = m_encoder.GetInstructionByteSize(opcode);
        BinaryInstruction result = TranslateInstruction(opcode, args);

        if (opcode == ".BYTE") // Output an arbitrary number of bytes. Used for program data.
        {
            if (parsedArgs[1][0] == '"')
            {
                // If it's a string, remove "" characters.
                parsedArgs[1] = parsedArgs[1].substr(1, parsedArgs[1].length() - 2);
                binaryOutput.write(parsedArgs[1].c_str() + '\0', sizeArg);
            }
            else if (parsedArgs[1].find('.') != std::string::npos) // If it's floating-point data.
            {
                if (parsedArgs[1].back() == 'f')
                    binaryOutput << std::stof(parsedArgs[1]); // Suffix == f ? Treat as float.
                else
                    binaryOutput << std::stod(parsedArgs[1]); // No suffix? Treat as double.
            }
            else
            {
                // 64-bit constants in program memory is not supported right now.
                // Why would all this ever be needed anyway? (save for strings).
                binaryOutput << static_cast<uint32_t>(std::stoul(parsedArgs[1]));
            }
        }
        // If not a .BYTE directive, output instructions.
        else if (instrBytes <= 12)
        {
            binaryOutput << result.opcode;
            binaryOutput.write((char*)(result.instr), instrBytes - 1);            
        }
        else
        {
            InstructionError("Invalid instruction size: " << instrBytes);
            return;
        } 

        // Do a horrible attempt at formatting output into columns.
        if ((m_flags & FLAG_SHOW_TRANSLATION) /* && opcode != ".BYTE" */)
        {
            std::stringstream finalTranslation;
            finalTranslation << std::left << std::setw(9) << opcode;
            
            if (parsedArgs[0] != "")
                finalTranslation << std::left << std::setw(14) << args[0];
            if (parsedArgs[1] != "")
                finalTranslation << ',' << std::left << std::setw(10) << args[1];
            if (parsedArgs[2] != "")
                finalTranslation << ',' << std::left << std::setw(10) << args[2];

            char instrAddr[13];
            std::snprintf(instrAddr, 13, "[0x%.8X]", m_instrAddr);

            std::cout << std::setfill(' ') << std::left << std::setw(13) << instrAddr << std::setw(20) << line <<
                '|' << std::setw(36) << finalTranslation.str(); 
            std::cout << "(";

            printf("%02hhX", result.opcode);

            uint8_t* ptr = (uint8_t*)result.instr;
            while (ptr < ((uint8_t*)result.instr) + instrBytes)
                printf(" %02hhX", *ptr++);

            std::cout << ")" << std::endl;
        }

        m_instrAddr += instrBytes;
    }

    //---- PUBLIC --------------------------------------------------------------------------------//

    size_t Assembler::Assemble(std::istream& textInput, std::iostream& binaryOutput)
    {
        m_hasError = false;

        // Temporary stream of assembly source that can be altered by the first pass,
        // and then read from by the second pass.
        workingText.clear();

        if (m_flags & FLAG_SHOW_FIRST_PASS)
            std::cout << "-------- FIRST PASS START --------" << std::endl;
        
        std::string line;
        for (m_lineNbr = 1, m_instrAddr = 0; std::getline(textInput, line); m_lineNbr++)
        {
            if (m_flags & FLAG_SHOW_FIRST_PASS)
                std::cout << m_lineNbr << " [" << m_instrAddr << "]\t" << line << std::endl;
        
            FirstPassReadLine(line);
        }

        if (m_flags & FLAG_SHOW_FIRST_PASS)
            std::cout << "-------- FIRST PASS END --------" << std::endl;

        // Write the header data first.
        binaryOutput.write((const char*)&m_binHeader, sizeof(m_binHeader));

        if (m_flags & FLAG_SHOW_TRANSLATION)
            std::cout << "-------- SECOND PASS BEGIN --------" << std::endl;

        // Begin the second pass, which actually starts to ouput binary.
        m_instrAddr = 0;
        while (std::getline(workingText, line))
        {
            AssembleLine(line, binaryOutput);
        }

        if (m_flags & FLAG_SHOW_TRANSLATION)
            std::cout << "-------- SECOND PASS END --------" << std::endl;

        if (!(m_flags & FLAG_SUPPRESS_UNUSED_LABELS))
            m_labelDict.WarnAboutUnusedLabels();

        if (m_hasError)
            return 0;

        binaryOutput.flush();
        return m_instrAddr; // In bytes.
    }
}

using namespace Assembly;

int main(int argc, char* argv[])
{
    std::cout << (IsLittleEndian() ? "Little endian" : "Big endian") << std::endl;

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

    std::fstream outFile(outputPath, std::ios::binary | std::ios::out);
    if (!outFile.is_open())
    {
        std::cerr << "Could not create \"" << outputPath << "\"." << std::endl;
        return 0;
    }

    size_t binarySize = 0;
    try
    {
        // Assemble the inputFile stream of assembly text into the binary output file.
        // The byte cound does not account for the header.
        binarySize = assembler.Assemble(inputFile, outFile);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }   

    if (binarySize > 0)
        std::cout << "Assembly successful! Wrote " << binarySize << " bytes." << std::endl;
    else
        std::cout << "Assembly failed!" << std::endl;

    return 0;
}
