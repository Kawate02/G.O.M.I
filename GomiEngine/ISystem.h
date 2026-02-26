#pragma once
#include "ComponentType.h"

class World;

class ISystem
{
public:
	virtual void initialize(World& world) = 0;
	virtual void update(World& world, int frameCount, float deltaTime) = 0;
};