#ifndef INC_LABEL_HPP
#define INC_LABEL_HPP

#include "common.hpp"
#include <unordered_map>
#include <iostream>
#include <string>

namespace Assembly 
{
    struct Label
    {
        Address address;
        unsigned int refCount;

        Label() : address(0), refCount(0)
        {
        }

        Label(Address addr) : address(addr), refCount(0)
        {
        }
    };

    class LabelDictionary
    {
    private:
        std::unordered_map<std::string, Label> m_labels;

    public:
        LabelDictionary();

        bool RegisterLabel(const std::string& label, Address value);
        bool ResolveLabel(const std::string& label, Address& addressOut);
        void WarnAboutUnusedLabels() const;
    };
}

#endif // INC_LABEL_HPP
