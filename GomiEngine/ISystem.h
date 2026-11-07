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
	/// <summary>
	/// エディタで止めている間も動かすか(描画の命令を積むだけの System は true)
	/// </summary>
	virtual bool runsInEditMode() const { return false; }
	virtual ~ISystem() = default;
};

template <typename T>
std::function<std::unique_ptr<ISystem>()> System()
{
	return []() { return std::make_unique<T>(); };
}