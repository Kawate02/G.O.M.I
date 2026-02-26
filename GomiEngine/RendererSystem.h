#pragma once

#include "ISystem.h"
#include "RendererComponent.h"
#include "TransformComponent.h"
#include "RenderQueue.h"

class RendererSystem : public ISystem
{
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};