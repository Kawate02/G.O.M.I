#pragma once
#include "ISystem.h"

class PhysicsSystem : public ISystem
{
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};