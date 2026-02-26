#pragma once
#include "ISystem.h"
#include "CollisionContext.h"

class CollisionSystem : public ISystem
{
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};