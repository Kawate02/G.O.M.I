#pragma once

#include "ISystem.h"
#include "InputState.h"

class PlayerControlSystem : public ISystem
{
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};