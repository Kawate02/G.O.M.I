#include "CollisionRule.h"
#include "AssetDatabase.h"

#include <stdexcept>

CollisionRules& CollisionRules::getInstance()
{
	static CollisionRules instance;
	return instance;
}

std::map<RuleID, CollisionRule> CollisionRules::parse(const nlohmann::json& json)
{
	const std::string file = FILE_NAME;
	if (!json.is_object()) throw std::runtime_error(file + ": expected an object of rules ({ \"<rule id>\": { \"a\", \"b\", \"kind\" } })");
	std::map<RuleID, CollisionRule> table;
	for (auto& [id, body] : json.items())
	{
		try
		{
			if (!body.is_object()) throw std::runtime_error("expected { \"a\": <tag>, \"b\": <tag>, \"kind\": \"BLOCK\" | \"TRIGGER\" }");
			for (auto& [key, value] : body.items())
			{
				if (key != "a" && key != "b" && key != "kind") throw std::runtime_error("unknown field \"" + key + "\"");
			}
			auto tag = [&](const char* key) {
				auto it = body.find(key);
				if (it == body.end()) throw std::runtime_error(std::string("\"") + key + "\" is missing");
				if (!it->is_string()) throw std::runtime_error(std::string(key) + ": expected a tag name (string)");
				try { return TagRegistry::getInstance().get(it->get<std::string>()); }
				catch (const std::exception& e) { throw std::runtime_error(std::string(key) + ": " + e.what()); }
			};
			CollisionRule rule;
			rule.a = tag("a");
			rule.b = tag("b");
			auto kind = body.find("kind");
			if (kind == body.end()) throw std::runtime_error("\"kind\" is missing");
			if (!kind->is_string()) throw std::runtime_error("kind: expected \"BLOCK\" or \"TRIGGER\"");
			auto k = enumFromName<CollisionKind>(kind->get<std::string>());
			if (!k) throw std::runtime_error("kind: unknown kind \"" + kind->get<std::string>() + "\" (BLOCK or TRIGGER)");
			rule.kind = *k;
			table[id] = rule;
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error(file + ": " + id + ": " + e.what());
		}
	}
	return table;
}

const CollisionRule* CollisionRules::find(const RuleID& id) const
{
	auto it = rules.find(id);
	return it == rules.end() ? nullptr : &it->second;
}

void loadCollisionSettings(const nlohmann::json& project)
{
	auto& tags = TagRegistry::getInstance();
	auto oldTags = tags.getNames();
	tags.loadProject(project);
	try
	{
		auto& db = AssetDatabase::getInstance();
		std::map<RuleID, CollisionRule> table;
		if (db.exists(CollisionRules::FILE_NAME)) table = CollisionRules::parse(db.readJson(CollisionRules::FILE_NAME));
		CollisionRules::getInstance().set(std::move(table));
	}
	catch (...)
	{
		tags.set(oldTags);
		throw;
	}
}
