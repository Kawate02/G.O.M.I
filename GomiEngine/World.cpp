#include "World.h"
#include "SceneLoader.h"
#include "SystemRegistry.h"

void World::initialize(const Json& scene, const std::string& source)
{
	SceneLoader::load(*this, scene, source);
	uiManager.initialize();
	for (auto& it : systems)
	{
		it->initialize(*this);
	}
}

void World::addSystem(const std::string& name)
{
	systems.push_back(SystemRegistry::getInstance().create(name));
	systemNames.push_back(name);
}

void World::update(float dt, bool simulate)
{
	deltaTime = dt;
	simulating = simulate;
	if (!simulate)
	{
		uiManager.update(inputState, renderQueue, false);
		for (auto& it : systems)
		{
			if (it->runsInEditMode()) it->update(*this, frameCount, 0.0f);
		}
		flushDestroyed();
		eventBus.clearEvents();
		return;
	}
	frameCount++;
	if (inputState.isDownKey(InputKey::KEY_ESCAPE) && !pause)
	{
		if (!pauseUIPath.empty())
		{
			pauseUI = spawnUI(pauseUIPath, { 0, 0 }, 1);
			pause = true;
		}
	}
	uiManager.update(inputState, renderQueue);
	if (pauseClose.trigger())
	{
		uiManager.remove(pauseUI);
		pause = false;
	}
	if (pause) return;
	collision.createGrid(*this);
	for (auto& it : systems)
	{
		it->update(*this, frameCount, deltaTime);
	}
	flushDestroyed();
	eventBus.clearEvents();
}

void World::release()
{
	component.releaseAll();
	for (auto& it : systems)
	{
		it.reset();
	}
	entities.clear();
	generations.clear();
	pendingDestroy.clear();
	prefabLinks.clear();
	std::queue<uint32_t>().swap(freeIDs);
}

UIHandle World::spawnUI(const AssetID& prefab, Vec2 pos, int layer)
{
	return SceneLoader::spawnUI(uiManager, AssetDatabase::getInstance().readJson(prefab), pos, layer, prefab);
}

Entity World::createEntity()
{
	uint32_t id;
	if (freeIDs.empty())
	{
		id = static_cast<uint32_t>(generations.size());
		generations.push_back(0);
		entities.resize(generations.size());
	}
	else
	{
		id = freeIDs.front();
		freeIDs.pop();
	}
	entities[id] = { id, generations[id] };
	return { id, generations[id] };
}

void World::destroyEntity(Entity e)
{
	if (!alive(e)) return;
	generations[e.id]++;
	pendingDestroy.push_back(e);
}

void World::flushDestroyed()
{
	for (auto& e : pendingDestroy)
	{
		component.removeAll(e);
		prefabLinks.erase(e.id);
		freeIDs.push(e.id);
	}
	pendingDestroy.clear();
}
Json World::runState() const
{
	return { { "frameCount", frameCount }, { "pause", pause } };
}

void World::restoreRunState(const Json& state)
{
	frameCount = state.value("frameCount", 0);
	bool paused = state.value("pause", false);
	if (paused && !pause && !pauseUIPath.empty())
	{
		pauseUI = spawnUI(pauseUIPath, { 0, 0 }, 1);
		pause = true;
	}
}
