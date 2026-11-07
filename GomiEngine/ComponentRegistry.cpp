#include "ComponentRegistry.h"

const FieldInfo* ComponentInfo::findField(std::string_view fieldName) const
{
	for (auto& f : fields)
	{
		if (f.name == fieldName) return &f;
	}
	return nullptr;
}

const ComponentInfo* ComponentRegistry::find(std::string_view name) const
{
	for (auto& c : components)
	{
		if (c.name == name) return &c;
	}
	return nullptr;
}

const ComponentInfo* ComponentRegistry::find(std::type_index type) const
{
	for (auto& c : components)
	{
		if (c.type == type) return &c;
	}
	return nullptr;
}
