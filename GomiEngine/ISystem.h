#pragma once
#include "ComponentType.h"

#include <memory>
#include <functional>

class World;

class ISystem
{
public:
	virtual void initialize(World& world) = 0;
	virtual void update(World& world, int frameCount, float deltaTime) = 0;
};

template <typename T>
std::function<std::unique_ptr<ISystem>()> System()
{
	return []() { return std::make_unique<T>(); };
}