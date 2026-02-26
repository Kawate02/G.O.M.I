#pragma once
#include "ISystem.h"

class BackGroundSystem : public ISystem
{
private:
	float posMod(float pos, float size);
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};
