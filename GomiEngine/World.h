#pragma once
#include "Utilities.h"
#include "SystemInclude.h"
#include "ComponentManager.h"
#include "UIManager.h"
#include "WorldEventBus.h"
#include "Camera.h"
#include "Prefab.h"
#include "UIPrefab.h"
#include "CollisionContext.h"

#include "InputState.h"
#include "RenderQueue.h"
#include "AudioQueue.h"
#include "EventRequest.h"

using StageID = int;

struct StageDefinition
{
	std::string name;
	std::vector<std::function<std::unique_ptr<ISystem>()>> systems;
	std::vector<SpawnOverride> prefabs;
	std::vector<UISpawnOverride> uiPrefabs;
};

class World
{
protected:
	Camera camera;
	ComponentManager component;
	CollisionContext collision;
	UIManager uiManager;
	std::vector<std::unique_ptr<ISystem>> systems;
	InputState& inputState;
	RenderQueue& renderQueue;
	AudioQueue& audioQueue;
	WorldEventBus eventBus;

	std::vector<Entity> entities;
	std::vector<uint32_t> generations;
	std::queue<uint32_t> freeIDs;

	EventRequest<int> pauseClose{ Event::Pause_Close };
	UIHandle pauseUI = { 0, 0 };
	bool pause = false;

	int frameCount = 0;
	float deltaTime = 0.0f;
public:
	World(InputState& input, RenderQueue& rend, AudioQueue& aud) : inputState(input), renderQueue(rend), audioQueue(aud) {}
	void initialize(const StageDefinition& prefab);
	void update(float dt);
	void release();
	Entity createEntity();
	void destroyEntity(Entity e);
	UIHandle spawnUIPrefab(const UIPrefabDefinition& prefab, Vec2 pos, int layer);
	inline ComponentManager& getComponentManager() { return component; }
	inline Camera& getCamera() { return camera; }
	inline bool alive(Entity e) { return e.generation == generations[e.id]; }
	inline WorldEventBus& event() { return eventBus; }
	inline CollisionContext& col() { return collision; }
	inline const std::vector<Entity>& getEntities() { return entities; }
	inline InputState& getInputState() { return inputState; }
	inline RenderQueue& getRenderQueue() { return renderQueue; }
	inline AudioQueue& getAudioQueue() { return audioQueue; }
	inline UIManager& getUIManager() { return uiManager; }
};