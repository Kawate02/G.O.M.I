#pragma once
#include "Camera.h"
#include "ISystem.h"

class CameraSystem : public ISystem
{
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};