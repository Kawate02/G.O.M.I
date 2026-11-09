#include "SceneDiff.h"
#include "AssetDatabase.h"
#include "SceneLoader.h"

#include <map>
#include <set>

namespace
{
	std::string nameOf(const Json& components)
	{
		if (components.contains("Name") && components["Name"].contains("name") && components["Name"]["name"].is_string())
			return components["Name"]["name"].get<std::string>();
		return "";
	}

	std::string fileName(const std::string& path)
	{
		return path.substr(path.find_last_of('/') + 1);
	}
}

std::string SceneDiff::entityLabel(const Json& components, const std::string& prefab, int index, int members)
{
	std::string name = nameOf(components);
	if (!name.empty()) return name;
	if (!prefab.empty())
	{
		std::string file = fileName(prefab);
		if (file.size() > 5 && file.compare(file.size() - 5, 5, ".json") == 0) file.resize(file.size() - 5);
		return members > 1 ? file + " [" + std::to_string(index) + "]" : file;
	}
	auto field = [&](const char* comp, const char* key) -> std::string {
		if (components.contains(comp) && components[comp].contains(key) && components[comp][key].is_string())
			return components[comp][key].get<std::string>();
		return "";
	};
	if (components.contains("Camera3D")) return "Camera3D";
	if (components.contains("Light")) return "Light";
	if (std::string mesh = field("MeshRenderer", "mesh"); !mesh.empty()) return mesh.substr(mesh.find_last_of("/:") + 1);
	if (std::string image = field("Renderer", "path"); !image.empty()) return fileName(image);
	return "Entity";
}

std::vector<std::string> SceneDiff::numbered(const std::vector<std::string>& labels)
{
	std::map<std::string, int> seen;
	std::vector<std::string> out;
	for (auto& label : labels)
	{
		int n = ++seen[label];
		out.push_back(n > 1 ? label + " #" + std::to_string(n) : label);
	}
	return out;
}

std::string SceneDiff::shortValue(const Json& value, size_t limit)
{
	std::string s = value.is_null() ? "(none)" : value.dump();
	if (s.size() > limit) s = s.substr(0, limit) + "...";
	return s;
}

std::string SceneChange::text() const
{
	switch (kind)
	{
	case Kind::EntityAdded: return "+ " + entity;
	case Kind::EntityRemoved: return "- " + entity;
	case Kind::ComponentAdded: return entity + ": + " + component;
	case Kind::ComponentRemoved: return entity + ": - " + component;
	case Kind::FieldChanged: return entity + " > " + component + "." + field + ": " + SceneDiff::shortValue(before) + " -> " + SceneDiff::shortValue(after);
	case Kind::SystemsChanged: return "systems: " + SceneDiff::shortValue(before) + " -> " + SceneDiff::shortValue(after);
	case Kind::UIChanged: return "ui: " + SceneDiff::shortValue(before) + " -> " + SceneDiff::shortValue(after);
	}
	return "";
}

Json SceneDiff::expand(const Json& scene)
{
	Json out = Json::array();
	std::vector<std::string> labels;
	auto push = [&](std::string label, Json components) {
		labels.push_back(label);
		out.push_back({ { "label", label }, { "components", std::move(components) } });
	};
	if (!scene.is_object() || !scene.contains("entities") || !scene["entities"].is_array()) return out;
	auto finish = [&] {
		auto names = numbered(labels);
		for (size_t i = 0; i < names.size(); i++) out[i]["label"] = names[i];
		return out;
	};
	for (auto& item : scene["entities"])
	{
		if (!item.is_object()) continue;
		if (item.contains("prefab") && item["prefab"].is_string())
		{
			AssetID id = item["prefab"].get<std::string>();
			Json defs;
			try
			{
				defs = AssetDatabase::getInstance().readJson(id).value("entities", Json::array());
			}
			catch (const std::exception&)
			{
				push(entityLabel(Json::object(), id) + " (unreadable)", Json::object());
				continue;
			}
			for (size_t i = 0; i < defs.size(); i++)
			{
				Json comps = defs[i].value("components", Json::object());
				if (i == 0 && item.contains("overrides")) comps = SceneLoader::merge(comps, item["overrides"]);
				try
				{
					comps = SceneLoader::normalize(comps);
				}
				catch (const std::exception&)
				{
				}
				push(entityLabel(comps, id, static_cast<int>(i), static_cast<int>(defs.size())), comps);
			}
		}
		else
		{
			Json comps = item.value("components", Json::object());
			try
			{
				comps = SceneLoader::normalize(comps);
			}
			catch (const std::exception&)
			{
			}
			push(entityLabel(comps), comps);
		}
	}
	return finish();
}

std::vector<SceneChange> SceneDiff::compare(const Json& before, const Json& after)
{
	using Kind = SceneChange::Kind;
	std::vector<SceneChange> changes;
	auto get = [](const Json& scene, const char* key) {
		return scene.is_object() && scene.contains(key) ? scene[key] : Json::array();
	};
	if (get(before, "systems") != get(after, "systems"))
		changes.push_back({ Kind::SystemsChanged, "", -1, "", "", get(before, "systems"), get(after, "systems") });

	Json a = expand(before), b = expand(after);
	//呼び名で対応させ、残りは同じ並び順同士
	std::vector<int> match(b.size(), -1);
	std::vector<bool> used(a.size(), false);
	std::map<std::string, int> byLabel;
	for (size_t i = 0; i < a.size(); i++) byLabel[a[i]["label"].get<std::string>()] = static_cast<int>(i);
	for (size_t j = 0; j < b.size(); j++)
	{
		auto it = byLabel.find(b[j]["label"].get<std::string>());
		if (it != byLabel.end() && !used[it->second])
		{
			match[j] = it->second;
			used[it->second] = true;
		}
	}
	for (size_t j = 0; j < b.size(); j++)
	{
		if (match[j] < 0 && j < a.size() && !used[j])
		{
			match[j] = static_cast<int>(j);
			used[j] = true;
		}
	}

	for (size_t j = 0; j < b.size(); j++)
	{
		std::string label = b[j]["label"].get<std::string>();
		int index = static_cast<int>(j);
		if (match[j] < 0)
		{
			changes.push_back({ Kind::EntityAdded, label, index });
			continue;
		}
		const Json& ca = a[match[j]]["components"];
		const Json& cb = b[j]["components"];
		std::set<std::string> names;
		for (auto& [k, v] : ca.items()) names.insert(k);
		for (auto& [k, v] : cb.items()) names.insert(k);
		for (auto& name : names)
		{
			if (!cb.contains(name))
			{
				changes.push_back({ Kind::ComponentRemoved, label, index, name });
				continue;
			}
			if (!ca.contains(name))
			{
				changes.push_back({ Kind::ComponentAdded, label, index, name });
				continue;
			}
			const Json& fa = ca[name];
			const Json& fb = cb[name];
			if (fa == fb) continue;
			if (!fa.is_object() || !fb.is_object())
			{
				changes.push_back({ Kind::FieldChanged, label, index, name, "", fa, fb });
				continue;
			}
			std::set<std::string> fields;
			for (auto& [k, v] : fa.items()) fields.insert(k);
			for (auto& [k, v] : fb.items()) fields.insert(k);
			for (auto& field : fields)
			{
				Json va = fa.contains(field) ? fa[field] : Json();
				Json vb = fb.contains(field) ? fb[field] : Json();
				if (va != vb) changes.push_back({ Kind::FieldChanged, label, index, name, field, va, vb });
			}
		}
	}
	for (size_t i = 0; i < a.size(); i++)
	{
		if (!used[i]) changes.push_back({ Kind::EntityRemoved, a[i]["label"].get<std::string>(), -1 });
	}

	if (get(before, "ui") != get(after, "ui"))
		changes.push_back({ Kind::UIChanged, "", -1, "", "", get(before, "ui"), get(after, "ui") });
	return changes;
}
