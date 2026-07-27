#include "WorldManager.h"

void WorldManager::initialize()
{
	createWorld(0);
}

void WorldManager::createWorld(int next)
{
	crrentStageID = next;
	crrentWorld = std::make_unique<World>(inputState, renderQueue, audioQueue);
	auto& prefab = stageTable.at(next);
	crrentWorld->initialize(prefab);
}

void WorldManager::changeWorld(int next)
{
	crrentWorld->release();
	crrentWorld = std::make_unique<World>(inputState, renderQueue, audioQueue);
	crrentStageID = next;
	auto& prefab = stageTable.at(next);
	crrentWorld->initialize(prefab);
}

void WorldManager::update(float deltaTime)
{
	if (std::string name; worldChangeRequest.trigger(name))
	{
		changeWorld(worldNames.at(name));
	}
	crrentWorld->update(deltaTime);
}

std::string WorldManager::addWorld(const std::string& name, const StageDefinition& stageDef)
{
	StageID id = 0;
	while (stageTable.find(id) != stageTable.end())
	{
		id++;
	}
	stageTable[id] = stageDef;
	worldNames[name] = id;
	return name;
}

std::string WorldManager::addWorld(const StageDefinition& stageDef)
{
	return addWorld(stageDef.name, stageDef);
}
