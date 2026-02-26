#pragma once
#include "ISystem.h"
#include "MoveEvent.h"

class MoveSystem : public ISystem
{
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};