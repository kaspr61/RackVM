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

#ifndef INC_CLI_ARGS_HPP
#define INC_CLI_ARGS_HPP

#include <string>
#include <memory>
#include <unordered_map>

namespace CLIArgs
{
    enum class ArgType
    {
        NONE, INT, FLOAT, STRING
    } ;


    struct Arg
    {
        int intVal;
        float floatVal;
        std::string strVal;
        std::string id;
        ArgType valueType;

        Arg() : 
            id(""),
            valueType(ArgType::NONE),
            intVal(0),
            floatVal(0),
            strVal()
        {
        }

        Arg(const std::string& id) : 
            id(id),
            valueType(ArgType::NONE),
            intVal(0),
            floatVal(0),
            strVal()
        {
        }

        ~Arg()
        {
        }

        int Get(int def) const { return valueType == ArgType::INT ? intVal : def; }
        float Get(float def) const { return valueType == ArgType::FLOAT ? intVal : def; }
        std::string Get(const std::string& def) const { return valueType == ArgType::STRING ? strVal : def; }
        bool Get(bool def) const { return true; }
    };

    struct ArgInfo
    {
        const std::string id;
        const ArgType type;
        const std::string description;

        ArgInfo(std::string&& id, ArgType type) :
            id(std::move(id)),
            type(type),
            description("")
        {
        }

        ArgInfo(std::string&& id, ArgType type, std::string&& description) :
            id(std::move(id)),
            type(type),
            description(std::move(description))
        {
        }
    };

    class ArgParser
    {
    private:
        std::unordered_map<std::string, Arg> m_args;
        std::unordered_map<std::string, ArgInfo> m_validArgs;

    public:
        ArgParser(std::initializer_list<ArgInfo>&& validArgs) :
            m_args()
        {
            for (auto arg : validArgs)
                m_validArgs.emplace(arg.id, std::move(arg));
        }

        ArgParser(const std::initializer_list<ArgInfo>& validArgs) :
            m_args()
        {
            for (auto arg : validArgs)
                m_validArgs.emplace(arg.id, std::move(arg));
        }

        bool Parse(int argc, char* argv[]) 
        {
            for (int i = 1; i < argc; i++)
            {
                if (*argv[i] != '-') // Ignore arguments that don't start with '-'.
                    continue;

                std::string argId(argv[i]);
                argId = argId.substr(0, argId.find('='));
                
                char* argValue = std::strchr(argv[i], '=') + 1;

                auto arg = m_validArgs.find(argId);
                if (arg == m_validArgs.end())
                {
                    std::cerr << "Invalid argument \"" << argId << "\"." << std::endl;
                    return false; 
                }

                ArgType argType = arg->second.type;

                switch (argType)
                {
                    case ArgType::NONE:
                        Set(argId);
                        break;
                    case ArgType::INT:
                        Set(argId, std::stoi(argValue));
                        break;
                    case ArgType::FLOAT:
                        Set(argId, std::stof(argValue));
                        break;
                    case ArgType::STRING:
                        Set(argId, std::string(argValue)); 
                        break;
                    default:
                        std::cerr << "Unknown argument type on \"" << argId << "\"." << std::endl;
                        return false;
                }
            }

            return true; 
        }

        template<typename T>
        T Get(const std::string& id, T def) const
        {
            auto it = m_args.find(id);
            if (it != m_args.cend())
                return it->second.Get(def);
            
            return def;
        }

        void Set(const std::string& id)
        {
            Arg arg(id);
            m_args.insert(std::make_pair(id, arg));
        }

        void Set(const std::string& id, int value)
        {
            Arg arg(id);
            arg.valueType = ArgType::INT;
            arg.intVal = value;
            m_args.insert(std::make_pair(id, arg));
        }

        void Set(const std::string& id, float value)
        {
            Arg arg(id);
            arg.valueType = ArgType::FLOAT;
            arg.floatVal = value;
            m_args.insert(std::make_pair(id, arg));
        }

        void Set(const std::string& id, std::string&& value)
        {
            Arg arg(id);
            arg.valueType = ArgType::STRING;
            arg.strVal = std::move(value);
            m_args.insert(std::make_pair(id, arg));
        }

        void Set(const std::string& id, const std::string& value)
        {
            Arg arg(id);
            arg.valueType = ArgType::STRING;
            arg.strVal = value;
            m_args.insert(std::make_pair(id, arg));
        }
    };
}

#endif // INC_CLI_ARGS_HPP
