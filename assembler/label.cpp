#include "label.hpp"

namespace Assembly
{
	LabelDictionary::LabelDictionary()
	{
		// Add 32 registers as labels.
		for (int i = 0; i < 32; i++)
			m_labels["R"+std::to_string(i)] = Label(i);
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
		auto isRegister = [](const std::string& label) { 
			if (label[0] == 'R' && label.length() > 1 && std::isdigit(label[1]))
				return true;

			return false;
		};

		for (auto elem : m_labels)
		{
			if (elem.second.refCount == 0 && !isRegister(elem.first))
			{
				std::cout << "Warning: unused label \"" << elem.first << "\"." << std::endl;
			}
		}
	}
}
