#include "ResourceRegistry.h"
#include "AssetDatabase.h"

const FieldInfo* ResourceInfo::findField(std::string_view fieldName) const
{
	for (auto& f : fields)
	{
		if (f.name == fieldName) return &f;
	}
	return nullptr;
}

Json ResourceInfo::toJson(const void* value) const
{
	Json j = Json::object();
	for (auto& f : fields) j[f.name] = f.save(value);
	return j;
}

void ResourceInfo::applyJson(void* value, const Json& j) const
{
	if (!j.is_object()) throw std::runtime_error(name + ": expected an object");
	for (auto& [key, v] : j.items())
	{
		const FieldInfo* f = findField(key);
		if (!f) throw std::runtime_error(name + ": unknown field \"" + key + "\"");
		try { f->load(value, v); }
		catch (const std::exception& ex) { throw std::runtime_error(name + "." + key + ": " + ex.what()); }
	}
}

ResourceRegistry& ResourceRegistry::getInstance()
{
	static ResourceRegistry instance;
	return instance;
}

const ResourceInfo* ResourceRegistry::find(std::string_view name) const
{
	for (auto& r : resources)
	{
		if (r.name == name) return &r;
	}
	return nullptr;
}

const ResourceInfo* ResourceRegistry::find(std::type_index type) const
{
	for (auto& r : resources)
	{
		if (r.type == type) return &r;
	}
	return nullptr;
}

void ResourceRegistry::truncate(size_t count)
{
	if (count < resources.size()) resources.erase(resources.begin() + count, resources.end());
}

void* ResourceStore::slot(const ResourceInfo& info)
{
	auto it = values.find(info.type);
	if (it == values.end()) it = values.emplace(info.type, info.create()).first;
	return it->second.get();
}

void* ResourceStore::find(std::string_view name)
{
	const ResourceInfo* info = ResourceRegistry::getInstance().find(name);
	return info ? slot(*info) : nullptr;
}

void ResourceStore::reset(const Json& initial, const std::string& source)
{
	auto& registry = ResourceRegistry::getInstance();
	if (!initial.is_object()) throw std::runtime_error(source + ": expected an object { \"ResourceName\": { ... } }");
	//全部作ってから入れ替える(途中で間違いがあれば今の値のまま)
	std::unordered_map<std::type_index, std::shared_ptr<void>> next;
	for (auto& info : registry.all()) next.emplace(info.type, info.create());
	for (auto& [name, fields] : initial.items())
	{
		const ResourceInfo* info = registry.find(name);
		if (!info) throw std::runtime_error(source + ": unknown resource \"" + name + "\"");
		try { info->applyJson(next.at(info->type).get(), fields); }
		catch (const std::exception& ex) { throw std::runtime_error(source + ": " + ex.what()); }
	}
	values = std::move(next);
}

void ResourceStore::resetToInitial()
{
	auto& db = AssetDatabase::getInstance();
	reset(db.exists(INITIAL_FILE) ? db.readJson(INITIAL_FILE) : Json::object(), INITIAL_FILE);
}

void ResourceStore::restore(const Json& state)
{
	values.clear();
	if (!state.is_object()) return;
	for (auto& info : ResourceRegistry::getInstance().all())
	{
		auto it = state.find(info.name);
		if (it == state.end() || !it->is_object()) continue;
		void* value = slot(info);
		for (auto& [key, v] : it->items())
		{
			const FieldInfo* f = info.findField(key);
			if (!f) continue;
			try { f->load(value, v); }
			catch (const std::exception&) {}
		}
	}
}

Json ResourceStore::save()
{
	Json j = Json::object();
	for (auto& info : ResourceRegistry::getInstance().all()) j[info.name] = info.toJson(slot(info));
	return j;
}
