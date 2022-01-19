#include <iostream>
#include <fstream>
#include <string>

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

    std::string line;
    for (int lineNbr = 1, instrNbr = 0; std::getline(inputFile, line); lineNbr++)
    {
        uint32_t binCode = 0x0;

        std::cout << lineNbr << " [" << instrNbr << "]\t" << line << "\t" << (binCode > 0 ? std::to_string(binCode) : "") << std::endl;
    }

    return 0;
}
