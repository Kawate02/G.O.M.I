#include "SceneEditor.h"
#include "ComponentRegistry.h"

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
	project = db.exists("project.json") ? db.readJson("project.json") : Json::object();
	newScene();
}

void SceneEditor::rebuild(const Json& scene)
{
	auto next = std::make_unique<World>(inputState, renderQueue, audioQueue);
	next->setPauseUI(project.value("pauseUI", std::string()));
	next->initialize(scene, path.empty() ? "(new scene)" : path);
	if (currentWorld) currentWorld->release();
	currentWorld = std::move(next);
}

Json SceneEditor::snapshot()
{
	return SceneLoader::save(*currentWorld);
}

// ---- ファイル

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
	rebuild(scene);
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
		Json data = AssetDatabase::getInstance().readJson(id);
		AssetID old = path;
		path = id;
		try
		{
			rebuild(data);
		}
		catch (...)
		{
			path = old;
			throw;
		}
	}
	catch (const std::exception& e)
	{
		error = e.what();
		return false;
	}
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
	try
	{
		AssetDatabase::getInstance().writeJson(id, snapshot());
	}
	catch (const std::exception& e)
	{
		error = e.what();
		return false;
	}
	path = id;
	dirty = false;
	return true;
}

// ---- Entity

std::vector<Entity> SceneEditor::entities()
{
	std::vector<Entity> result;
	for (auto& e : currentWorld->getEntities())
	{
		if (currentWorld->alive(e)) result.push_back(e);
	}
	return result;
}

std::string SceneEditor::displayName(Entity e)
{
	auto& cm = currentWorld->getComponentManager();
	if (cm.has<NameComponent>(e))
	{
		auto& name = cm.getComponentArray<NameComponent>().getComponent(e).name;
		if (!name.empty()) return name;
	}
	if (const PrefabLink* link = currentWorld->getPrefabLink(e))
	{
		std::string file = link->prefab.substr(link->prefab.find_last_of('/') + 1);
		return file + " [" + std::to_string(link->index) + "]";
	}
	//名前が無ければ、役目の分かるコンポーネントから付ける
	if (cm.has<Camera3DComponent>(e)) return "Camera3D";
	if (cm.has<LightComponent>(e)) return "Light";
	if (cm.has<MeshRendererComponent>(e))
	{
		auto& mesh = cm.getComponentArray<MeshRendererComponent>().getComponent(e).mesh;
		return mesh.substr(mesh.find_last_of("/:") + 1);
	}
	if (cm.has<RendererComponent>(e))
	{
		auto& image = cm.getComponentArray<RendererComponent>().getComponent(e).path;
		return image.substr(image.find_last_of('/') + 1);
	}
	return "Entity " + std::to_string(e.id);
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
		undoStack.push_back(before);
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
	undoStack.push_back(snapshot());
	if (undoStack.size() > MAX_UNDO) undoStack.erase(undoStack.begin());
	redoStack.clear();
	dirty = true;
}

void SceneEditor::undo()
{
	if (!canUndo()) return;
	redoStack.push_back(snapshot());
	Json scene = undoStack.back();
	undoStack.pop_back();
	rebuild(scene);
	dirty = true;
}

void SceneEditor::redo()
{
	if (!canRedo()) return;
	undoStack.push_back(snapshot());
	Json scene = redoStack.back();
	redoStack.pop_back();
	rebuild(scene);
	dirty = true;
}

// ---- 再生

void SceneEditor::play()
{
	if (playing) return;
	playSnapshot = snapshot();
	playing = true;
	paused = false;
	stepRequested = false;
	playFrame = 0;
	worldChange.trigger();
	quitRequest.trigger();
}

void SceneEditor::stop()
{
	if (!playing) return;
	playing = false;
	paused = false;
	stepRequested = false;
	playFrame = 0;
	rebuild(playSnapshot);
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
	bool advance = playing && (!paused || stepRequested);
	stepRequested = false;
	if (advance) playFrame++;
	currentWorld->update(deltaTime, advance);
}
