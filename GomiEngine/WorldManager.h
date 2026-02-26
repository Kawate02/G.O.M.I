#pragma once
#include "World.h"
#include "EventRequest.h"

class WorldManager
{
private:
	EventRequest<int> worldChangeRequest{Event::World_Change};
	int crrentStageID = -1;
	std::unique_ptr<World> crrentWorld;

	InputState& inputState;
	RenderQueue& renderQueue;
	AudioQueue& audioQueue;
public:
	WorldManager(InputState& input, RenderQueue& rend, AudioQueue& audio) : inputState(input), renderQueue(rend), audioQueue(audio) {}
	void initialize();
	void createWorld(int next);
	void changeWorld(int next);
	void update(float deltaTime);
	inline void release()
	{
		crrentWorld->release();
	}
	inline World& getWorld() { return *crrentWorld; }
};