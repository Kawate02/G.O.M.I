#pragma once
#include "World.h"
#include "EventRequest.h"

class WorldManager
{
private:
	std::unordered_map<StageID, StageDefinition> stageTable;
	std::unordered_map<std::string, StageID> worldNames;
	EventRequest<std::string> worldChangeRequest{Event::World_Change};
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
	std::string addWorld(const std::string& name, const StageDefinition& stageDef);
	std::string addWorld(const StageDefinition& stageDef);
	inline World& getWorld() { return *crrentWorld; }
};