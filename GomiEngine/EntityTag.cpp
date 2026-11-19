#include "EntityTag.h"

#include <algorithm>
#include <stdexcept>

TagSet TagSet::of(std::string_view name)
{
	return TagRegistry::getInstance().get(name);
}

TagRegistry& TagRegistry::getInstance()
{
	static TagRegistry instance;
	return instance;
}

void TagRegistry::set(std::vector<std::string> list)
{
	if (list.size() > MAX_TAGS) throw std::runtime_error("too many tags (" + std::to_string(list.size()) + "): up to " + std::to_string(MAX_TAGS));
	for (size_t i = 0; i < list.size(); i++)
	{
		if (list[i].empty()) throw std::runtime_error("[" + std::to_string(i) + "]: empty tag name");
		if (std::find(list.begin(), list.begin() + i, list[i]) != list.begin() + i) throw std::runtime_error("[" + std::to_string(i) + "]: tag \"" + list[i] + "\" is listed twice");
	}
	names = std::move(list);
}

void TagRegistry::loadProject(const nlohmann::json& project)
{
	std::vector<std::string> list;
	try
	{
		auto it = project.find("tags");
		if (it != project.end())
		{
			if (!it->is_array()) throw std::runtime_error("expected an array of tag names");
			for (size_t i = 0; i < it->size(); i++)
			{
				if (!(*it)[i].is_string()) throw std::runtime_error("[" + std::to_string(i) + "]: expected a tag name (string)");
				list.push_back((*it)[i].get<std::string>());
			}
		}
		set(std::move(list));
	}
	catch (const std::exception& e)
	{
		throw std::runtime_error(std::string("project.json: tags: ") + e.what());
	}
}

std::optional<TagSet> TagRegistry::find(std::string_view name) const
{
	for (size_t i = 0; i < names.size(); i++)
	{
		if (names[i] == name) return TagSet{ 1u << i };
	}
	return std::nullopt;
}

TagSet TagRegistry::get(std::string_view name) const
{
	if (auto t = find(name)) return *t;
	throw std::runtime_error("unknown tag \"" + std::string(name) + "\" (add it to \"tags\" in project.json)");
}

std::vector<std::string> TagRegistry::toNames(TagSet tags) const
{
	std::vector<std::string> out;
	for (size_t i = 0; i < names.size(); i++)
	{
		if (tags.bits & (1u << i)) out.push_back(names[i]);
	}
	return out;
}
