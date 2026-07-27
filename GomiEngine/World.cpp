#include "World.h"
#include "UIPrefab_Pause.h"

void World::initialize(const StageDefinition& prefabs)
{
	for (auto& it : prefabs.systems)
	{
		systems.push_back(it());
	}
	for (auto& it : prefabs.prefabs)
	{
		bool first = true;
		for (auto& entityDef : it.prefab->entities)
		{
			Entity e = createEntity();
			for (auto& addComp : entityDef.components)
			{
				addComp(component, e);
			}
			if (first && it.pos.has_value())
			{
				component.addComponent(e, TransformComponent({ *it.pos, it.scale, it.rot }));
			}
			first = false;
		}
	}
	for (auto& it : prefabs.uiPrefabs)
	{
		spawnUIPrefab(*it.prefab, it.pos, it.layer);
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
		pauseUI = spawnUIPrefab(UIPrefab_Pause, { 0, 0 }, 1);
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

UIHandle World::spawnUIPrefab(const UIPrefabDefinition& prefab, Vec2 pos, int layer)
{
	UIHandle root = uiManager.add(std::make_unique<UIBase>(Vec2{ 0, 0 }, Vec2{ 0, 0 }, layer, 0, false));
	for (auto& child : prefab.children)
	{
		uiManager.addChild(root, child.factory(), child.type);
	}
	uiManager.move(root, pos);
	return root;
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