#include "SceneLoader.h"
#include "World.h"
#include "ComponentRegistry.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"

#include <initializer_list>
#include <stdexcept>

namespace
{
	[[noreturn]] void fail(const std::string& where, const std::string& what)
	{
		throw std::runtime_error(where + ": " + what);
	}

	/// <summary>
	/// 知らないキー(打ち間違い)があれば例外にする
	/// </summary>
	void checkKeys(const Json& j, std::initializer_list<const char*> allowed, const std::string& where)
	{
		if (!j.is_object()) fail(where, "expected an object");
		for (auto& [key, v] : j.items())
		{
			bool ok = false;
			for (auto a : allowed) ok = ok || key == a;
			if (!ok) fail(where, "unknown key \"" + key + "\"");
		}
	}

	template <typename T>
	T value(const Json& j, const char* key, T fallback, const std::string& where)
	{
		auto it = j.find(key);
		if (it == j.end()) return fallback;
		try
		{
			T v = fallback;
			fromJson(*it, v);
			return v;
		}
		catch (const std::exception& e)
		{
			fail(where + "." + key, e.what());
		}
	}

	void addComponents(World& world, Entity e, const Json& components, const std::string& where)
	{
		if (!components.is_object()) fail(where, "\"components\" must be an object");
		auto& registry = ComponentRegistry::getInstance();
		for (auto& [name, fields] : components.items())
		{
			const ComponentInfo* info = registry.find(name);
			if (!info) fail(where, "unknown component \"" + name + "\"");
			try
			{
				info->addFromJson(world.getComponentManager(), e, fields);
			}
			catch (const std::exception& ex)
			{
				fail(where, ex.what());
			}
		}
	}

	void spawnEntity(World& world, const Json& item, const std::string& where)
	{
		checkKeys(item, { "prefab", "overrides", "components" }, where);
		if (item.contains("prefab"))
		{
			if (item.contains("components")) fail(where, "use either \"prefab\" or \"components\"");
			AssetID prefabId = item["prefab"].get<std::string>();
			Json prefab = AssetDatabase::getInstance().readJson(prefabId);
			checkKeys(prefab, { "entities" }, prefabId);
			const Json& defs = prefab.value("entities", Json::array());
			bool first = true;
			int instance = world.newPrefabInstance();
			for (size_t i = 0; i < defs.size(); i++)
			{
				//overrides の間違いもあるので、シーン側の場所も付ける
				std::string at = where + " > " + prefabId + ": entities[" + std::to_string(i) + "]";
				checkKeys(defs[i], { "components" }, at);
				Json components = defs[i].value("components", Json::object());
				if (first && item.contains("overrides")) components = SceneLoader::merge(components, item["overrides"]);
				first = false;
				Entity e = world.createEntity();
				addComponents(world, e, components, at);
				world.setPrefabLink(e, { prefabId, static_cast<int>(i), instance });
			}
		}
		else
		{
			if (item.contains("overrides")) fail(where, "\"overrides\" needs \"prefab\"");
			addComponents(world, world.createEntity(), item.value("components", Json::object()), where);
		}
	}

	std::function<void()> makeAction(const Json& onClick, const std::string& where)
	{
		if (onClick.is_null()) return nullptr;
		checkKeys(onClick, { "event", "value" }, where);
		if (!onClick.contains("event")) fail(where, "\"event\" is required");
		std::string eventName = onClick["event"].get<std::string>();
		Json v = onClick.value("value", Json(0));
		//受け取る側の EventRequest<T> と型を合わせる(文字列なら std::string、数なら int)
		if (v.is_string())
		{
			std::string s = v.get<std::string>();
			return [eventName, s]() { EventBus::getInstance().emit<std::string>(eventName, s); };
		}
		if (v.is_number_integer())
		{
			int n = v.get<int>();
			return [eventName, n]() { EventBus::getInstance().emit<int>(eventName, n); };
		}
		fail(where + ".value", "must be a string or an integer");
	}

	std::unique_ptr<UIBase> makeWidget(const Json& w, const std::string& where)
	{
		if (!w.is_object() || !w.contains("type")) fail(where, "\"type\" is required");
		std::string type = w["type"].get<std::string>();
		Vec2 pos = value(w, "pos", Vec2(0, 0), where);
		int layer = value(w, "layer", 0, where);
		int order = value(w, "order", 0, where);
		bool block = value(w, "block", false, where);
		if (type == "Text")
		{
			checkKeys(w, { "type", "tag", "pos", "layer", "order", "block", "text", "fontSize", "anchor" }, where);
			return std::make_unique<UIText>(value(w, "text", std::string(), where), value(w, "fontSize", 16, where),
				value(w, "anchor", Anchor::TOP_LEFT, where), pos, layer, order, block);
		}
		if (type == "Image")
		{
			checkKeys(w, { "type", "tag", "pos", "layer", "order", "block", "image", "scale" }, where);
			return std::make_unique<UIImage>(value(w, "image", std::string(), where), pos, value(w, "scale", Vec2(1, 1), where), layer, order, block);
		}
		if (type == "Button")
		{
			checkKeys(w, { "type", "tag", "pos", "layer", "order", "block", "idle", "hover", "pressed", "scale", "onClick", "hoverSound", "clickSound" }, where);
			auto button = std::make_unique<UIButton>(value(w, "idle", std::string(), where), value(w, "hover", std::string(), where),
				value(w, "pressed", std::string(), where), pos, value(w, "scale", Vec2(1, 1), where), layer, order,
				makeAction(w.value("onClick", Json()), where + ".onClick"), block);
			button->setSounds(value(w, "hoverSound", std::string("sounds/cursor_move.wav"), where),
				value(w, "clickSound", std::string("sounds/cursor_click.wav"), where));
			return button;
		}
		fail(where, "unknown widget type \"" + type + "\"");
	}
}

AssetID SceneLoader::scenePath(const std::string& nameOrPath)
{
	if (nameOrPath.size() >= 5 && nameOrPath.compare(nameOrPath.size() - 5, 5, ".json") == 0) return nameOrPath;
	return "scenes/" + nameOrPath + ".json";
}

Json SceneLoader::merge(Json base, const Json& patch)
{
	if (!patch.is_object() || !base.is_object()) return patch;
	for (auto& [key, v] : patch.items())
	{
		base[key] = base.contains(key) ? merge(base[key], v) : v;
	}
	return base;
}

void SceneLoader::load(World& world, const Json& scene, const std::string& source)
{
	checkKeys(scene, { "systems", "entities", "ui" }, source);

	const Json& systems = scene.value("systems", Json::array());
	for (size_t i = 0; i < systems.size(); i++)
	{
		std::string at = source + ": systems[" + std::to_string(i) + "]";
		if (!systems[i].is_string()) fail(at, "expected a system name");
		try { world.addSystem(systems[i].get<std::string>()); }
		catch (const std::exception& e) { fail(at, e.what()); }
	}

	const Json& entities = scene.value("entities", Json::array());
	for (size_t i = 0; i < entities.size(); i++)
	{
		spawnEntity(world, entities[i], source + ": entities[" + std::to_string(i) + "]");
	}

	const Json& ui = scene.value("ui", Json::array());
	for (size_t i = 0; i < ui.size(); i++)
	{
		std::string at = source + ": ui[" + std::to_string(i) + "]";
		checkKeys(ui[i], { "prefab", "pos", "layer" }, at);
		if (!ui[i].contains("prefab")) fail(at, "\"prefab\" is required");
		world.spawnUI(ui[i]["prefab"].get<std::string>(), value(ui[i], "pos", Vec2(0, 0), at), value(ui[i], "layer", 0, at));
	}
	world.setUIEntries(ui);
}

Json SceneLoader::componentsOf(World& world, Entity e)
{
	auto& cm = world.getComponentManager();
	Json components = Json::object();
	for (auto& info : ComponentRegistry::getInstance().all())
	{
		if (info.has(cm, e)) components[info.name] = info.toJson(cm, e);
	}
	return components;
}

Json SceneLoader::normalize(const Json& components)
{
	ComponentManager cm;
	Entity e{ 0, 0 };
	Json out = Json::object();
	for (auto& [name, fields] : components.items())
	{
		const ComponentInfo* info = ComponentRegistry::getInstance().find(name);
		if (!info) continue;
		info->addFromJson(cm, e, fields);
		out[name] = info->toJson(cm, e);
	}
	return out;
}

Json SceneLoader::diff(const Json& base, const Json& current)
{
	if (!base.is_object() || !current.is_object()) return base == current ? Json() : current;
	Json out = Json::object();
	for (auto& [key, v] : current.items())
	{
		if (!base.contains(key))
		{
			out[key] = v;
			continue;
		}
		Json d = diff(base[key], v);
		if (!d.is_null() && !(d.is_object() && d.empty() && v.is_object())) out[key] = d;
	}
	return out;
}

std::vector<Entity> SceneLoader::instantiate(World& world, const AssetID& prefabId, const Json& overrides)
{
	Json item = { { "prefab", prefabId } };
	if (!overrides.is_null() && !overrides.empty()) item["overrides"] = overrides;
	std::vector<Entity> created;
	spawnEntity(world, item, prefabId);
	//新しく作った Entity(空き番号の再利用もあるので、つながりで探す)
	for (auto& e : world.getEntities())
	{
		const PrefabLink* link = world.alive(e) ? world.getPrefabLink(e) : nullptr;
		if (link && link->prefab == prefabId) created.push_back(e);
	}
	int newest = 0;
	for (auto& e : created) newest = std::max(newest, world.getPrefabLink(e)->instance);
	std::vector<Entity> result;
	for (auto& e : created)
	{
		if (world.getPrefabLink(e)->instance == newest) result.push_back(e);
	}
	std::sort(result.begin(), result.end(), [&](Entity a, Entity b) { return world.getPrefabLink(a)->index < world.getPrefabLink(b)->index; });
	return result;
}

Json SceneLoader::save(World& world)
{
	Json scene = Json::object();
	scene["systems"] = world.getSystemNames();

	//プレハブの各 Entity のコンポーネントを、既定値を埋めた形にして覚えておく
	std::unordered_map<AssetID, std::vector<Json>> prefabCache;
	auto prefabEntities = [&](const AssetID& id) -> const std::vector<Json>& {
		auto it = prefabCache.find(id);
		if (it != prefabCache.end()) return it->second;
		std::vector<Json> list;
		try
		{
			Json prefab = AssetDatabase::getInstance().readJson(id);
			for (auto& def : prefab.value("entities", Json::array())) list.push_back(normalize(def.value("components", Json::object())));
		}
		catch (const std::exception&)
		{
			//プレハブが消えていたら、つながりを切って展開して書く
		}
		return prefabCache[id] = list;
	};

	//同じ配置(instance)の仲間を集める
	std::unordered_map<int, std::vector<Entity>> instances;
	for (auto& e : world.getEntities())
	{
		if (!world.alive(e)) continue;
		if (const PrefabLink* link = world.getPrefabLink(e)) instances[link->instance].push_back(e);
	}

	Json entities = Json::array();
	std::unordered_map<int, bool> written;
	for (auto& e : world.getEntities())
	{
		if (!world.alive(e)) continue;
		const PrefabLink* link = world.getPrefabLink(e);
		if (!link)
		{
			entities.push_back({ { "components", componentsOf(world, e) } });
			continue;
		}
		if (written.count(link->instance)) continue;
		written[link->instance] = true;

		//プレハブ + 差分で書けるか: 全員そろっていて、2番目以降は変わっておらず、1番目はコンポーネントが減っていない
		auto members = instances[link->instance];
		std::sort(members.begin(), members.end(), [&](Entity a, Entity b) { return world.getPrefabLink(a)->index < world.getPrefabLink(b)->index; });
		const auto& defs = prefabEntities(link->prefab);
		bool keep = !defs.empty() && members.size() == defs.size();
		Json overrides;
		for (size_t i = 0; keep && i < members.size(); i++)
		{
			const PrefabLink* m = world.getPrefabLink(members[i]);
			Json current = componentsOf(world, members[i]);
			if (m->index != static_cast<int>(i) || m->prefab != link->prefab) keep = false;
			else if (i == 0)
			{
				for (auto& [name, v] : defs[0].items()) keep = keep && current.contains(name);
				overrides = diff(defs[0], current);
			}
			else if (current != defs[i]) keep = false;
		}
		if (keep)
		{
			Json item = { { "prefab", link->prefab } };
			if (overrides.is_object() && !overrides.empty()) item["overrides"] = overrides;
			entities.push_back(item);
		}
		else
		{
			for (auto& m : members) entities.push_back({ { "components", componentsOf(world, m) } });
		}
	}
	scene["entities"] = entities;
	scene["ui"] = world.getUIEntries();
	return scene;
}

UIHandle SceneLoader::spawnUI(UIManager& ui, const Json& prefab, Vec2 pos, int layer, const std::string& source)
{
	checkKeys(prefab, { "widgets" }, source);
	//子をまとめて動かすための見えない親
	UIHandle root = ui.add(std::make_unique<UIBase>(Vec2{ 0, 0 }, Vec2{ 0, 0 }, layer, 0, false));
	const Json& widgets = prefab.value("widgets", Json::array());
	for (size_t i = 0; i < widgets.size(); i++)
	{
		std::string at = source + ": widgets[" + std::to_string(i) + "]";
		auto widget = makeWidget(widgets[i], at);
		ui.addChild(root, std::move(widget), value(widgets[i], "tag", UIType::NONE, at));
	}
	ui.move(root, pos);
	return root;
}
