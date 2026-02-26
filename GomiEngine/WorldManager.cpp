#include "WorldManager.h"

void WorldManager::initialize()
{
	createWorld(0);
}

void WorldManager::createWorld(int next)
{
	crrentStageID = next;
	crrentWorld = std::make_unique<World>(inputState, renderQueue, audioQueue);
	crrentWorld->initialize(next);
}

void WorldManager::changeWorld(int next)
{
	crrentWorld->release();
	crrentWorld = std::make_unique<World>(inputState, renderQueue, audioQueue);
	crrentStageID = next;
	crrentWorld->initialize(next);
}

void WorldManager::update(float deltaTime)
{
	if (int value = -1 ; worldChangeRequest.trigger(value))
	{
		changeWorld(value);
	}
	crrentWorld->update(deltaTime);
}
