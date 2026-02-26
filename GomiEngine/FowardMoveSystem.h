#pragma once
#include "ISystem.h"

class FowardMoveSystem : public ISystem
{
private:
	float WrapPi(float angle);
	float FacingForward(float current, float target, float maxDelta);
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
};