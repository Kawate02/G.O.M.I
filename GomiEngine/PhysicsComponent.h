#pragma once
#include "Vec2.h"

struct PhysicsComponent
{
	Vec2 gravVel = { 0, 0 };
	Vec2 extraVel = { 0, 0 };
	float gravity = 2400;
	float maxGravity = 3200;
	float decay = 3000;
	bool isGround = true;
};