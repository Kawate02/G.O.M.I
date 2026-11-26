#include "Project.h"
#include "SceneEditor.h"
#include "ComponentRegistry.h"
#include "AnimationClip.h"
#include "SpriteAnimationComponent.h"
#include "RendererComponent.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace
{
	const std::vector<std::string> defaultSystems = {
		"PlayerControlSystem", "PhysicsSystem", "MoveResolveSystem", "MoveSystem",
		"CameraSystem", "MeshRendererSystem", "RendererSystem",
	};
}

SceneEditor::SceneEditor(InputState& input, RenderQueue& render, AudioQueue& audio)
	: inputState(input), renderQueue(render), audioQueue(audio)
{
	auto& db = AssetDatabase::getInstance();
	project = Project::getInstance().read();
	try { loadCollisionSettings(project); }
	catch (const std::exception& e) { std::cerr << e.what() << std::endl; } //直して保存すれば checkFiles で読み直す
	newScene();
	loadResources(resourceError);
}

void SceneEditor::rebuild(const Json& scene)
{
	preview = AnimationPreview(); //World ごと作り直すので、戻す先が無い
	auto next = std::make_unique<World>(inputState, renderQueue, audioQueue, resources);
	next->setPauseUI(project.value("pauseUI", std::string()));
	next->initialize(scene, path.empty() ? "(new scene)" : path);
	if (currentWorld) currentWorld->release();
	currentWorld = std::move(next);
	//作った時のプレハブの中身を覚える(外でプレハブが変わっても、overrides を自分の変更だけにする)
	for (auto& e : currentWorld->getEntities())
	{
		if (const PrefabLink* link = currentWorld->getPrefabLink(e)) prefabDefs(link->prefab);
	}
}

Json SceneEditor::undoEntry(const Json& scene)
{
	return { { "scene", scene }, { "resources", resources.save() }, { "resourcesDirty", resourcesDirty } };
}

void SceneEditor::applyUndoEntry(const Json& entry)
{
	rebuild(entry.at("scene"));
	resources.restore(entry.at("resources"));
	resourcesDirty = entry.value("resourcesDirty", true);
}

/// <summary>
/// 書き出す間だけ、プレビューで書き換えた値を元に戻す(シーンの JSON にプレビューの途中の time・画像を入れない)
/// </summary>
struct SceneEditor::PreviewHold
{
	SceneEditor& editor;
	bool held;
	explicit PreviewHold(SceneEditor& e) : editor(e), held(e.preview.active)
	{
		if (held) editor.swapPreviewValues();
	}
	~PreviewHold()
	{
		if (held) editor.swapPreviewValues();
	}
};

Json SceneEditor::snapshot()
{
	PreviewHold hold(*this);
	return SceneLoader::save(*currentWorld, [this](const AssetID& id) { return prefabDefs(id); });
}

const std::vector<Json>& SceneEditor::prefabDefs(const AssetID& prefab)
{
	auto it = prefabCache.find(prefab);
	if (it == prefabCache.end()) it = prefabCache.emplace(prefab, SceneLoader::prefabComponents(prefab)).first;
	return it->second;
}

// ---- ファイル

bool SceneEditor::isPrefabPath(const AssetID& id)
{
	return id.rfind("prefabs/", 0) == 0;
}

Json SceneEditor::wrapPrefab(const Json& prefab) const
{
	//プレハブは Entity の並びだけなので、見て動かすための System を足してシーンにする
	return { { "systems", defaultSystems }, { "entities", prefab.value("entities", Json::array()) }, { "ui", Json::array() } };
}

Json SceneEditor::readAsScene(const AssetID& id)
{
	Json data = AssetDatabase::getInstance().readJson(id);
	if (!isPrefabPath(id)) return data;
	if (!data.is_object() || !data.contains("entities")) throw std::runtime_error(id + ": a prefab needs \"entities\"");
	for (auto& [key, v] : data.items())
	{
		if (key != "entities") throw std::runtime_error(id + ": unknown key in a prefab: \"" + key + "\"");
	}
	return wrapPrefab(data);
}

Json SceneEditor::fileContent(const AssetID& id)
{
	PreviewHold hold(*this);
	if (!isPrefabPath(id)) return snapshot();
	Json entitiesJson = Json::array();
	for (auto& e : entities()) entitiesJson.push_back({ { "components", SceneLoader::compact(SceneLoader::componentsOf(*currentWorld, e)) } });
	return { { "entities", entitiesJson } };
}

void SceneEditor::newScene()
{
	if (playing) stop();
	Json scene = { { "systems", defaultSystems }, { "entities", Json::array() }, { "ui", Json::array() } };
	scene["entities"].push_back({ { "components", {
		{ "Name", { { "name", "Camera" } } },
		{ "Transform", { { "position", { 0, 5, 10 } }, { "rotation", { -25, 0, 0 } }, { "scale", { 1, 1, 1 } } } },
		{ "Camera3D", Json::object() } } } });
	scene["entities"].push_back({ { "components", {
		{ "Name", { { "name", "Light" } } },
		{ "Light", Json::object() } } } });
	path.clear();
	prefabCache.clear();
	rebuild(scene);
	savedScene = snapshot();
	scenePending = false;
	undoStack.clear();
	redoStack.clear();
	dirty = false;
}

bool SceneEditor::open(const std::string& scene, std::string& error)
{
	if (playing) stop();
	AssetID id = SceneLoader::scenePath(scene);
	try
	{
		Json data = readAsScene(id);
		AssetID old = path;
		auto oldCache = prefabCache;
		path = id;
		prefabCache.clear();
		try
		{
			rebuild(data);
		}
		catch (...)
		{
			path = old;
			prefabCache = oldCache;
			throw;
		}
		savedScene = data;
	}
	catch (const std::exception& e)
	{
		error = e.what();
		return false;
	}
	scenePending = false;
	undoStack.clear();
	redoStack.clear();
	dirty = false;
	return true;
}

bool SceneEditor::save(std::string& error)
{
	if (path.empty())
	{
		error = "no file name (use Save As)";
		return false;
	}
	return saveAs(path, error);
}

bool SceneEditor::saveAs(const AssetID& id, std::string& error)
{
	if (playing)
	{
		error = "stop playing before saving";
		return false;
	}
	Json content = fileContent(id);
	try
	{
		AssetDatabase::getInstance().writeJson(id, content);
	}
	catch (const std::exception& e)
	{
		error = e.what();
		return false;
	}
	path = id;
	savedScene = isPrefabPath(id) ? wrapPrefab(content) : content;
	scenePending = false;
	rememberStamp(id); //自分の保存は「外で変わった」に数えない
	dirty = false;
	return true;
}

// ---- Entity

std::vector<Entity> SceneEditor::entities()
{
	return currentWorld->getEntities();
}

std::vector<std::string> SceneEditor::displayNames()
{
	std::vector<std::string> labels;
	for (auto& e : entities())
	{
		Json comps = SceneLoader::componentsOf(*currentWorld, e);
		if (const PrefabLink* link = currentWorld->getPrefabLink(e))
			labels.push_back(SceneDiff::entityLabel(comps, link->prefab, link->index, static_cast<int>(prefabDefs(link->prefab).size())));
		else labels.push_back(SceneDiff::entityLabel(comps));
	}
	return SceneDiff::numbered(labels);
}

std::string SceneEditor::displayName(Entity e)
{
	auto list = entities();
	auto names = displayNames();
	for (size_t i = 0; i < list.size(); i++)
	{
		if (list[i].id == e.id) return names[i];
	}
	return "Entity";
}

bool SceneEditor::saveAsPrefab(Entity e, const AssetID& id, std::string& error)
{
	if (playing || !currentWorld->alive(e))
	{
		error = playing ? "stop playing first" : "no entity";
		return false;
	}
	if (!isPrefabPath(id))
	{
		error = "a prefab must be under prefabs/: " + id;
		return false;
	}
	PreviewHold hold(*this);
	Json file = { { "entities", Json::array({ { { "components", SceneLoader::compact(SceneLoader::componentsOf(*currentWorld, e)) } } }) } };
	try
	{
		AssetDatabase::getInstance().writeJson(id, file);
	}
	catch (const std::exception& ex)
	{
		error = ex.what();
		return false;
	}
	rememberStamp(id);
	recordUndo();
	//その Entity を新しいプレハブの配置にする(配置の番号は今ある物の次)
	int instance = 0;
	for (auto& other : entities())
	{
		if (const PrefabLink* l = currentWorld->getPrefabLink(other)) instance = std::max(instance, l->instance + 1);
	}
	prefabCache.erase(id);
	prefabDefs(id);
	currentWorld->setPrefabLink(e, PrefabLink{ id, 0, instance });
	return true;
}

Json SceneEditor::copyEntity(Entity e)
{
	PreviewHold hold(*this);
	if (!currentWorld->alive(e)) return Json();
	Json comps = SceneLoader::componentsOf(*currentWorld, e);
	if (const PrefabLink* link = currentWorld->getPrefabLink(e))
	{
		const auto& defs = prefabDefs(link->prefab);
		bool missing = false;
		if (!defs.empty())
		{
			for (auto& [name, v] : defs[0].items()) missing = missing || !comps.contains(name);
		}
		if (defs.size() == 1 && link->index == 0 && !missing)
		{
			Json item = { { "prefab", link->prefab } };
			Json overrides = SceneLoader::diff(defs[0], comps);
			if (overrides.is_object() && !overrides.empty()) item["overrides"] = overrides;
			return item;
		}
	}
	return { { "components", SceneLoader::compact(comps) } };
}

bool SceneEditor::pasteEntities(const Json& items, int& count, std::string& error)
{
	count = 0;
	if (playing)
	{
		error = "stop playing first";
		return false;
	}
	Json list = items.is_array() ? items : Json::array({ items });
	//{ "Transform": {...} } のようにコンポーネントだけ渡されたら components に包む
	for (auto& item : list)
	{
		if (item.is_object() && !item.contains("components") && !item.contains("prefab")) item = { { "components", item } };
	}
	size_t countBefore = entities().size();
	Json before = snapshot();
	Json scene = before;
	for (auto& item : list) scene["entities"].push_back(item);
	try
	{
		rebuild(scene);
	}
	catch (const std::exception& e)
	{
		error = e.what();
		rebuild(before);
		return false;
	}
	undoStack.push_back(undoEntry(before));
	if (undoStack.size() > MAX_UNDO) undoStack.erase(undoStack.begin());
	redoStack.clear();
	dirty = true;
	count = static_cast<int>(entities().size() - countBefore);
	return true;
}

void SceneEditor::rename(Entity e, const std::string& name)
{
	if (!currentWorld->alive(e)) return;
	recordUndo();
	auto& cm = currentWorld->getComponentManager();
	if (cm.has<NameComponent>(e)) cm.getComponentArray<NameComponent>().getComponent(e).name = name;
	else cm.addComponent(e, NameComponent{ name });
}

Entity SceneEditor::createEntity(const std::string& name)
{
	recordUndo();
	Entity e = currentWorld->createEntity();
	auto& cm = currentWorld->getComponentManager();
	cm.addComponent(e, NameComponent{ name });
	cm.addComponent(e, TransformComponent{ Vec3(), Vec3(1, 1, 1), Vec3() });
	return e;
}

std::vector<Entity> SceneEditor::createFromPrefab(const AssetID& prefab)
{
	Json before = snapshot();
	try
	{
		auto made = SceneLoader::instantiate(*currentWorld, prefab);
		undoStack.push_back(undoEntry(before));
		if (undoStack.size() > MAX_UNDO) undoStack.erase(undoStack.begin());
		redoStack.clear();
		dirty = true;
		return made;
	}
	catch (const std::exception& e)
	{
		std::cerr << "cannot place prefab: " << e.what() << std::endl;
		rebuild(before); //途中まで作った Entity を消す
		return {};
	}
}

Entity SceneEditor::duplicate(Entity e)
{
	recordUndo();
	Json components = SceneLoader::componentsOf(*currentWorld, e);
	Entity copy = currentWorld->createEntity();
	auto& registry = ComponentRegistry::getInstance();
	for (auto& [name, fields] : components.items())
	{
		registry.find(name)->addFromJson(currentWorld->getComponentManager(), copy, fields);
	}
	//1体だけのプレハブから作ったものなら、複製もそのプレハブの別の配置にする
	if (const PrefabLink* link = currentWorld->getPrefabLink(e))
	{
		int count = 0;
		for (auto& other : entities())
		{
			const PrefabLink* l = currentWorld->getPrefabLink(other);
			if (l && l->instance == link->instance) count++;
		}
		if (count == 1) currentWorld->setPrefabLink(copy, { link->prefab, link->index, currentWorld->newPrefabInstance() });
	}
	return copy;
}

void SceneEditor::destroy(Entity e)
{
	if (!currentWorld->alive(e)) return;
	recordUndo();
	currentWorld->destroyEntity(e);
	currentWorld->update(0.0f, false); //破棄をすぐ反映する
}

// ---- コンポーネント

bool SceneEditor::addComponent(Entity e, const std::string& component)
{
	const ComponentInfo* info = ComponentRegistry::getInstance().find(component);
	if (!info || info->has(currentWorld->getComponentManager(), e)) return false;
	recordUndo();
	info->addFromJson(currentWorld->getComponentManager(), e, Json::object());
	return true;
}

void SceneEditor::removeComponent(Entity e, const std::string& component)
{
	const ComponentInfo* info = ComponentRegistry::getInstance().find(component);
	if (!info || !info->has(currentWorld->getComponentManager(), e)) return;
	recordUndo();
	info->remove(currentWorld->getComponentManager(), e);
}

bool SceneEditor::setField(Entity e, const std::string& component, const std::string& field, const Json& value, std::string* error)
{
	const ComponentInfo* info = ComponentRegistry::getInstance().find(component);
	if (!info) return false;
	stopAnimationPreview();
	void* ptr = info->get(currentWorld->getComponentManager(), e);
	const FieldInfo* f = info->findField(field);
	if (!ptr || !f) return false;
	try
	{
		f->load(ptr, value);
	}
	catch (const std::exception& ex)
	{
		if (error) *error = ex.what();
		return false;
	}
	if (!playing) dirty = true;
	return true;
}

Json SceneEditor::getField(Entity e, const std::string& component, const std::string& field)
{
	const ComponentInfo* info = ComponentRegistry::getInstance().find(component);
	if (!info) return Json();
	void* ptr = info->get(currentWorld->getComponentManager(), e);
	const FieldInfo* f = info->findField(field);
	if (!ptr || !f) return Json();
	return f->save(ptr);
}

Json SceneEditor::prefabField(Entity e, const std::string& component, const std::string& field)
{
	const PrefabLink* link = currentWorld->getPrefabLink(e);
	if (!link) return Json();
	const auto& defs = prefabDefs(link->prefab);
	if (link->index < 0 || link->index >= static_cast<int>(defs.size())) return Json();
	const Json& comps = defs[link->index];
	if (!comps.contains(component) || !comps[component].contains(field)) return Json();
	return comps[component][field];
}

bool SceneEditor::revertToPrefab(Entity e, const std::string& component, const std::string& field)
{
	Json value = prefabField(e, component, field);
	if (value.is_null()) return false;
	recordUndo();
	return setField(e, component, field, value);
}

// ---- 保存済みとの差・外で変わったファイル

std::vector<SceneChange> SceneEditor::changesFromSaved()
{
	return SceneDiff::compare(savedScene, playing ? playSnapshot : snapshot());
}

std::map<AssetID, SceneEditor::FileStamp> SceneEditor::scanFiles() const
{
	namespace fs = std::filesystem;
	std::map<AssetID, FileStamp> stamps;
	const fs::path& root = AssetDatabase::getInstance().getRoot();
	std::error_code ec;
	for (fs::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec))
	{
		if (!it->is_regular_file(ec) || it->path().extension() != ".json") continue;
		FileStamp s;
		s.time = it->last_write_time(ec);
		s.size = it->file_size(ec);
		stamps[fs::relative(it->path(), root, ec).generic_string()] = s;
	}
	//project.json は assets の外(プロジェクト直下)にある
	auto projectFile = Project::getInstance().file();
	if (fs::is_regular_file(projectFile, ec))
	{
		FileStamp s;
		s.time = fs::last_write_time(projectFile, ec);
		s.size = fs::file_size(projectFile, ec);
		stamps[std::string("../") + Project::FILE_NAME] = s;
	}
	return stamps;
}

void SceneEditor::rememberStamp(const AssetID& id)
{
	if (!filesWatched) return;
	std::error_code ec;
	auto file = AssetDatabase::getInstance().resolve(id);
	FileStamp s;
	s.time = std::filesystem::last_write_time(file, ec);
	if (!ec) s.size = std::filesystem::file_size(file, ec);
	if (!ec) fileStamps[id] = s;
}

SceneEditor::FileCheck SceneEditor::checkFiles()
{
	FileCheck result;
	if (playing) return result; //止めた時に調べる
	auto now = scanFiles();
	if (!filesWatched)
	{
		fileStamps = std::move(now);
		filesWatched = true;
		return result;
	}
	for (auto& [id, stamp] : now)
	{
		auto it = fileStamps.find(id);
		if (it == fileStamps.end() || !(it->second == stamp)) result.changed.push_back(id);
	}
	for (auto& [id, stamp] : fileStamps)
	{
		if (!now.count(id)) result.changed.push_back(id);
	}
	fileStamps = std::move(now);
	if (result.changed.empty()) return result;
	//resources.json が外で変わった: 保存していない変更が無ければ読み直す
	if (!resourcesDirty && std::find(result.changed.begin(), result.changed.end(), ResourceStore::INITIAL_FILE) != result.changed.end())
	{
		std::string resourceLoadError;
		if (loadResources(resourceLoadError)) result.refreshed = true;
		else result.error = resourceLoadError;
	}

	bool sceneHit = false;
	bool others = false;
	for (auto& id : result.changed)
	{
		if (!path.empty() && id == path) sceneHit = true;
		else others = true;
	}

	//プレハブ・UI・project.json: 自分の変更を残したまま作り直す
	if (others)
	{
		Json scene = snapshot(); //読み込んだ時のプレハブとの差で書く
		auto oldCache = prefabCache;
		Json oldProject = project;
		auto oldTags = TagRegistry::getInstance().getNames();
		auto oldRules = CollisionRules::getInstance().all();
		auto& db = AssetDatabase::getInstance();
		prefabCache.clear();
		try
		{
			project = Project::getInstance().read();
			loadCollisionSettings(project); //project.json の tags・collision_rules.json
			rebuild(scene);
			result.refreshed = true;
		}
		catch (const std::exception& e)
		{
			prefabCache = oldCache;
			project = oldProject;
			TagRegistry::getInstance().set(oldTags);
			CollisionRules::getInstance().set(oldRules);
			result.error = e.what();
		}
	}

	if (sceneHit)
	{
		auto& db = AssetDatabase::getInstance();
		if (!db.exists(path))
		{
			result.error = path + ": the file was removed (save to write it again)";
			dirty = true;
			return result;
		}
		Json disk;
		try
		{
			disk = readAsScene(path);
		}
		catch (const std::exception& e)
		{
			result.error = e.what();
			return result;
		}
		if (disk == savedScene) return result; //中身は同じ
		diskScene = disk;
		if (dirty)
		{
			scenePending = true;
			result.sceneConflict = true;
			return result;
		}
		std::string error;
		if (reloadFromFile(error)) result.reloaded = true;
		else result.error = error;
	}
	return result;
}

std::vector<SceneChange> SceneEditor::fileChanges()
{
	return SceneDiff::compare(savedScene, diskScene);
}

bool SceneEditor::reloadFromFile(std::string& error)
{
	if (playing) stop();
	if (path.empty())
	{
		error = "no file to reload";
		return false;
	}
	Json disk;
	try
	{
		disk = readAsScene(path);
	}
	catch (const std::exception& e)
	{
		error = e.what();
		return false;
	}
	Json before = snapshot();
	try
	{
		rebuild(disk);
	}
	catch (const std::exception& e)
	{
		error = e.what();
		return false;
	}
	undoStack.push_back(undoEntry(before));
	if (undoStack.size() > MAX_UNDO) undoStack.erase(undoStack.begin());
	redoStack.clear();
	savedScene = disk;
	diskScene = disk;
	scenePending = false;
	dirty = false;
	return true;
}

void SceneEditor::keepEdits()
{
	savedScene = diskScene; //差分は「保存したら上書きされる所」になる
	scenePending = false;
	dirty = true;
}

// ---- System

std::vector<std::string> SceneEditor::systems()
{
	return currentWorld->getSystemNames();
}

void SceneEditor::setSystems(const std::vector<std::string>& names)
{
	recordUndo();
	Json scene = snapshot();
	scene["systems"] = names;
	try
	{
		rebuild(scene);
	}
	catch (const std::exception& e)
	{
		std::cerr << "cannot set systems: " << e.what() << std::endl;
		undo();
	}
}

// ---- 元に戻す

void SceneEditor::recordUndo()
{
	if (playing) return; //再生中の変更は止めた時に捨てる
	stopAnimationPreview(); //編集の前にプレビューの書き換えを元に戻す
	undoStack.push_back(undoEntry(snapshot()));
	if (undoStack.size() > MAX_UNDO) undoStack.erase(undoStack.begin());
	redoStack.clear();
	dirty = true;
}

void SceneEditor::recordResourceUndo()
{
	if (playing) return;
	undoStack.push_back(undoEntry(snapshot()));
	if (undoStack.size() > MAX_UNDO) undoStack.erase(undoStack.begin());
	redoStack.clear();
	resourcesDirty = true;
}

void SceneEditor::undo()
{
	if (!canUndo()) return;
	redoStack.push_back(undoEntry(snapshot()));
	Json entry = undoStack.back();
	undoStack.pop_back();
	applyUndoEntry(entry);
	dirty = true;
}

void SceneEditor::redo()
{
	if (!canRedo()) return;
	undoStack.push_back(undoEntry(snapshot()));
	Json entry = redoStack.back();
	redoStack.pop_back();
	applyUndoEntry(entry);
	dirty = true;
}

// ---- Resource

bool SceneEditor::loadResources(std::string& error)
{
	try
	{
		resources.resetToInitial();
	}
	catch (const std::exception& e)
	{
		error = resourceError = e.what();
		return false;
	}
	resourceError.clear();
	resourcesDirty = false;
	return true;
}

bool SceneEditor::saveResources(std::string& error)
{
	if (playing)
	{
		error = "stop playing before saving";
		return false;
	}
	try
	{
		AssetDatabase::getInstance().writeJson(ResourceStore::INITIAL_FILE, resources.save());
	}
	catch (const std::exception& e)
	{
		error = e.what();
		return false;
	}
	rememberStamp(ResourceStore::INITIAL_FILE); //自分の保存は「外で変わった」に数えない
	resourceError.clear();
	resourcesDirty = false;
	return true;
}

Json SceneEditor::getResourceField(const std::string& resource, const std::string& field)
{
	const ResourceInfo* info = ResourceRegistry::getInstance().find(resource);
	const FieldInfo* f = info ? info->findField(field) : nullptr;
	if (!f) return Json();
	return f->save(resources.find(resource));
}

bool SceneEditor::setResourceField(const std::string& resource, const std::string& field, const Json& value, std::string* error)
{
	const ResourceInfo* info = ResourceRegistry::getInstance().find(resource);
	const FieldInfo* f = info ? info->findField(field) : nullptr;
	if (!f)
	{
		if (error) *error = "unknown resource field " + resource + "." + field;
		return false;
	}
	void* target = resources.find(resource);
	Json before = f->save(target);
	try
	{
		f->load(target, value);
	}
	catch (const std::exception& e)
	{
		f->load(target, before);
		if (error) *error = e.what();
		return false;
	}
	if (!playing) resourcesDirty = true;
	return true;
}

// ---- 再生

void SceneEditor::play()
{
	if (playing || isPrefabMode()) return; //プレハブはシーンに置いてから動かす
	stopAnimationPreview();
	playSnapshot = snapshot();
	playResources = resources.save();
	playing = true;
	paused = false;
	stepRequested = false;
	playFrame = 0;
	playScene = sceneName(path);
	worldChange.trigger();
	quitRequest.trigger();
}

std::string SceneEditor::sceneName(const std::string& path)
{
	const std::string prefix = "scenes/", ext = ".json";
	if (path.size() > prefix.size() + ext.size() && path.compare(0, prefix.size(), prefix) == 0 && path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
		return path.substr(prefix.size(), path.size() - prefix.size() - ext.size());
	return path;
}

void SceneEditor::stop()
{
	if (!playing) return;
	playing = false;
	paused = false;
	stepRequested = false;
	playFrame = 0;
	rebuild(playSnapshot);
	resources.restore(playResources);
}

bool SceneEditor::swapCode(const std::function<bool(std::string& error)>& swap, const std::function<void()>& revert, std::string& error)
{
	if (playing)
	{
		error = "cannot swap the game code while playing";
		return false;
	}
	Json scene = snapshot();
	Json resourceValues = resources.save();
	//DLL の型の配列・System・Resource の値を持つものを先に全部消す(関数の実体が DLL の中にある)
	currentWorld->release();
	currentWorld.reset();
	prefabCache.clear();
	resources.clear();
	if (swap(error))
	{
		try
		{
			resources.restore(resourceValues);
			if (!resourcesDirty) loadResources(resourceError); //新しいコードの型で resources.json を読み直す
			rebuild(scene);
			return true;
		}
		catch (const std::exception& e)
		{
			error = e.what();
			if (currentWorld) currentWorld->release();
			currentWorld.reset();
			prefabCache.clear();
			resources.clear();
		}
	}
	revert();
	resources.restore(resourceValues);
	try { rebuild(scene); }
	catch (const std::exception& e)
	{
		error += std::string(" / cannot restore the scene: ") + e.what();
		newScene();
	}
	return false;
}

Json SceneEditor::playState()
{
	std::vector<Entity> order;
	Json world = SceneLoader::save(*currentWorld, &order);
	return { { "world", world }, { "resources", resources.save() }, { "scene", playScene }, { "frame", playFrame }, { "run", currentWorld->runState(order) } };
}

bool SceneEditor::restorePlayState(const Json& state, std::string& error)
{
	if (!playing)
	{
		error = "not playing";
		return false;
	}
	try
	{
		if (state.contains("resources")) resources.reset(state.at("resources"), "play state");
		rebuild(state.at("world"));
		currentWorld->restoreRunState(state.value("run", Json::object()));
	}
	catch (const std::exception& e)
	{
		error = e.what();
		return false;
	}
	playScene = state.value("scene", playScene);
	playFrame = state.value("frame", 0);
	stepRequested = false;
	return true;
}

void SceneEditor::setPaused(bool value)
{
	if (!playing) return;
	paused = value;
	stepRequested = false;
}

void SceneEditor::stepFrame()
{
	if (!playing) return;
	paused = true;
	stepRequested = true;
}

void SceneEditor::update(float deltaTime)
{
	if (playing)
	{
		//ゲームの中のシーン切り替え・終了は、エディタでは「別のシーンを試しに開く」「再生を止める」にする
		if (std::string next; worldChange.trigger(next))
		{
			try
			{
				rebuild(AssetDatabase::getInstance().readJson(SceneLoader::scenePath(next)));
				playScene = next;
			}
			catch (const std::exception& e)
			{
				std::cerr << "cannot change world: " << e.what() << std::endl;
			}
		}
		if (quitRequest.trigger())
		{
			stop();
			return;
		}
	}
	else
	{
		worldChange.trigger();
		quitRequest.trigger();
	}
	if (!playing && preview.active && preview.running)
	{
		auto& anims = currentWorld->getComponentManager().getComponentArray<SpriteAnimationComponent>();
		const AnimationClip* clip = nullptr;
		if (currentWorld->alive(preview.entity) && anims.hasComponent(preview.entity))
		{
			auto& anim = anims.getComponent(preview.entity);
			auto it = anim.clips.find(anim.clip);
			if (it != anim.clips.end()) clip = AnimationClip::load(it->second);
			if (clip)
			{
				anim.time += deltaTime * anim.speed;
				if (anim.time < 0) anim.time = 0;
				if (clip->loop && anim.time >= clip->duration()) anim.time = std::fmod(anim.time, clip->duration());
				if (!clip->loop && anim.time > clip->duration()) anim.time = clip->duration();
			}
		}
		if (clip) applyPreviewFrame();
		else preview = AnimationPreview(); //Entity かクリップが無くなった
	}
	bool advance = playing && (!paused || stepRequested);
	stepRequested = false;
	if (advance) playFrame++;
	currentWorld->update(deltaTime, advance);
}

// ---- アニメーションのプレビュー

void SceneEditor::swapPreviewValues()
{
	if (!preview.active || !currentWorld->alive(preview.entity)) return;
	auto& cm = currentWorld->getComponentManager();
	auto& anims = cm.getComponentArray<SpriteAnimationComponent>();
	auto& renderers = cm.getComponentArray<RendererComponent>();
	if (!anims.hasComponent(preview.entity) || !renderers.hasComponent(preview.entity)) return;
	auto& anim = anims.getComponent(preview.entity);
	auto& renderer = renderers.getComponent(preview.entity);
	std::swap(anim.clip, preview.clip);
	std::swap(anim.time, preview.time);
	std::swap(anim.playing, preview.playing);
	std::swap(renderer.path, preview.path);
	std::swap(renderer.uv, preview.uv);
}

void SceneEditor::applyPreviewFrame()
{
	auto& cm = currentWorld->getComponentManager();
	auto& anim = cm.getComponentArray<SpriteAnimationComponent>().getComponent(preview.entity);
	auto& renderer = cm.getComponentArray<RendererComponent>().getComponent(preview.entity);
	auto it = anim.clips.find(anim.clip);
	if (it == anim.clips.end()) return;
	const AnimationClip* clip = AnimationClip::load(it->second);
	if (!clip || clip->frames.empty()) return;
	const AnimationFrame& frame = clip->frames[clip->frameAt(anim.time)];
	renderer.path = frame.image;
	renderer.uv.assign(frame.uv.begin(), frame.uv.end());
}

bool SceneEditor::startAnimationPreview(Entity e, std::string& error)
{
	if (playing)
	{
		error = "stop playing first";
		return false;
	}
	if (preview.active && preview.entity == e) return true;
	if (!currentWorld->alive(e))
	{
		error = "no entity";
		return false;
	}
	auto& cm = currentWorld->getComponentManager();
	auto& anims = cm.getComponentArray<SpriteAnimationComponent>();
	auto& renderers = cm.getComponentArray<RendererComponent>();
	if (!anims.hasComponent(e) || !renderers.hasComponent(e))
	{
		error = "the entity needs both SpriteAnimation and Renderer";
		return false;
	}
	auto& anim = anims.getComponent(e);
	auto it = anim.clips.find(anim.clip);
	if (it == anim.clips.end())
	{
		error = "SpriteAnimation.clip \"" + anim.clip + "\" is not in clips";
		return false;
	}
	if (!AnimationClip::load(it->second))
	{
		error = "cannot read the clip: " + it->second;
		return false;
	}
	stopAnimationPreview();
	auto& renderer = renderers.getComponent(e);
	preview.active = true;
	preview.running = true;
	preview.entity = e;
	preview.clip = anim.clip;
	preview.time = anim.time;
	preview.playing = anim.playing;
	preview.path = renderer.path;
	preview.uv = renderer.uv;
	anim.time = 0;
	applyPreviewFrame();
	return true;
}

void SceneEditor::stopAnimationPreview()
{
	if (!preview.active) return;
	swapPreviewValues();
	preview = AnimationPreview();
}

void SceneEditor::setAnimationPreviewRunning(bool running)
{
	if (preview.active) preview.running = running;
}

bool SceneEditor::stepAnimationPreview(Entity e, int delta, std::string& error)
{
	if (!startAnimationPreview(e, error)) return false;
	preview.running = false;
	auto& anim = currentWorld->getComponentManager().getComponentArray<SpriteAnimationComponent>().getComponent(e);
	const AnimationClip* clip = AnimationClip::load(anim.clips.at(anim.clip));
	int count = static_cast<int>(clip->frames.size());
	int index = clip->frameAt(anim.time) + delta;
	if (clip->loop) index = ((index % count) + count) % count;
	else index = std::clamp(index, 0, count - 1);
	//そのコマの頭の時間にする(コマの長さ durations と fps から)
	float units = 0;
	for (int i = 0; i < index; i++) units += clip->durations[i];
	anim.time = units / clip->fps;
	applyPreviewFrame();
	return true;
}

int SceneEditor::animationPreviewFrame()
{
	if (!preview.active || !currentWorld->alive(preview.entity)) return -1;
	auto& anim = currentWorld->getComponentManager().getComponentArray<SpriteAnimationComponent>().getComponent(preview.entity);
	auto it = anim.clips.find(anim.clip);
	const AnimationClip* clip = it == anim.clips.end() ? nullptr : AnimationClip::load(it->second);
	return clip ? clip->frameAt(anim.time) : -1;
}

static bool writeClip(const AssetID& id, const Json& json, std::string& error)
{
	try
	{
		AnimationClip::parse(json, id); //書く前に、読み込みと同じ厳しさで調べる
		AssetDatabase::getInstance().writeJson(id, json);
		AnimationClip::clearCache();
		return true;
	}
	catch (const std::exception& e)
	{
		error = e.what();
		return false;
	}
}

bool SceneEditor::createSheetClip(const AssetID& id, const AssetID& sheet, int columns, int rows, int count, float fps, std::string& error)
{
	if (columns <= 0 || rows <= 0)
	{
		error = "columns and rows must be 1 or more";
		return false;
	}
	if (count <= 0) count = columns * rows;
	Json json = { { "sheet", sheet }, { "columns", columns }, { "rows", rows }, { "count", count }, { "fps", fps }, { "loop", true } };
	return writeClip(id, json, error);
}

bool SceneEditor::createSequenceClip(const AssetID& id, const std::string& imagePrefix, float fps, std::string& error)
{
	namespace fs = std::filesystem;
	auto& db = AssetDatabase::getInstance();
	fs::path prefix{ std::u8string(imagePrefix.begin(), imagePrefix.end()) };
	fs::path dir = db.getRoot() / prefix.parent_path();
	auto utf8 = [](const fs::path& p) { auto u = p.u8string(); return std::string(u.begin(), u.end()); };
	std::string stem = utf8(prefix.filename());
	std::vector<std::string> names;
	std::error_code ec;
	for (auto& entry : fs::directory_iterator(dir, ec))
	{
		std::string name = utf8(entry.path().filename());
		if (entry.path().extension() == ".png" && name.rfind(stem, 0) == 0) names.push_back(name);
	}
	if (names.empty())
	{
		error = "no images start with " + imagePrefix;
		return false;
	}
	std::sort(names.begin(), names.end()); //00, 01, … の連番は名前順で並ぶ
	std::string folder = imagePrefix.find('/') == std::string::npos ? std::string() : imagePrefix.substr(0, imagePrefix.rfind('/'));
	Json frames = Json::array();
	for (auto& n : names) frames.push_back(folder.empty() ? n : folder + "/" + n);
	Json json = { { "frames", frames }, { "fps", fps }, { "loop", true } };
	return writeClip(id, json, error);
}
