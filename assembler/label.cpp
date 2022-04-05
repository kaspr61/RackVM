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

#include "label.hpp"

namespace Assembly
{
	LabelDictionary::LabelDictionary()
	{
		// Add 32 registers as labels.
		for (int i = 0; i < 32; i++)
			m_labels["R"+std::to_string(i)] = Label(i);

		// Add system function labels. The address is in this 
		// only an index into the system function table of the VM.
		m_labels.insert({
			{"__print", Label(0)},
			{"__input", Label(1)},
			{"__write", Label(2)},
			{"__read",  Label(3)},
			{"__open",  Label(4)},
			{"__close", Label(5)},
		});
	}

	bool LabelDictionary::RegisterLabel(const std::string& label, Address value)
	{
		Label lbl;
		lbl.address = value;
		lbl.refCount = 0;

		auto ret = m_labels.insert(std::make_pair(label, lbl));
		bool wasCollision = !ret.second;
		if (wasCollision)
			return false;

		return true;
	}
	bool LabelDictionary::ResolveLabel(const std::string& label, Address& addressOut)
	{
		if (m_labels.count(label) < 1) // If label isn't defined.
			return false;

		Label& lbl = m_labels[label];
		lbl.refCount++;

		addressOut = lbl.address;
		return true;
	}

	void LabelDictionary::WarnAboutUnusedLabels() const
	{
		auto ignore = [](const std::string& label) 
		{ 
			// Ignore system function labels, as they are virtual labels. (Also main).
			if (label[0] == '_' && label[1] == '_' || label == "main")
				return true;

			// Ignore registers, they are not labels.
			if (label[0] == 'R' && label.length() > 1 && std::isdigit(label[1]))
				return true;

			return false;
		};

		for (auto elem : m_labels)
		{
			if (elem.second.refCount == 0 && !ignore(elem.first))
			{
				std::cout << "Warning: unused label \"" << elem.first << "\"." << std::endl;
			}
		}
	}
}
