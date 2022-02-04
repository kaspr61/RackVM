#ifndef INC_LABEL_HPP
#define INC_LABEL_HPP

#include "common.hpp"
#include <unordered_map>
#include <iostream>

namespace Assembly 
{
    struct Label
    {
        Address address;
        unsigned int refCount;
    };

    class LabelDictionary
    {
    private:
        std::unordered_map<std::string, Label> m_labels;

    public:
        bool RegisterLabel(const std::string& label, Address value);
        bool ResolveLabel(const std::string& label, Address& addressOut);

        void WarnAboutUnusedLabels() const;
    };
}

#endif // INC_LABEL_HPP
