#include "World.h"

void World::initialize(StageID stageID)
{
	auto& prefabs = stageTable.at(stageID);
	for (auto& it : prefabs.systems)
	{
		systems.push_back(createSystem(it));
	}
	for (auto& it : prefabs.prefabs)
	{
		prefabTable.at(it.prefab).func(*this, it);
	}
	for (auto& it : prefabs.uiPrefabs)
	{
		uiPrefabTable.at(it.prefab).func(uiManager, it);
	}
	uiManager.initialize();
	for (auto& it : systems)
	{
		it->initialize(*this);
	}
}

void World::update(float dt)
{
	deltaTime = dt;
	frameCount++;
	if (inputState.isDownKey(InputKey::KEY_ESCAPE) && !pause)
	{
		pauseUI = uiPrefabTable.at("Pause").func(uiManager, { "Pause", {0, 0}, 1 });
		pause = true;
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
	std::queue<uint32_t>().swap(freeIDs);
}

Entity World::createEntity()
{
	uint32_t id;
	if (freeIDs.empty())
	{
		id = generations.size();
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
	entities[e.id] = { e.id, e.generation };
	freeIDs.push(e.id);
}