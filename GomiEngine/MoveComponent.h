#pragma once
#include "Vec2.h"

struct MoveComponent
{
	Vec2 inputVel = {0, 0};
	Vec2 physVel = {0, 0};
	Vec2 result = {0, 0};
	float speed;
	float maxSpeed;
	float stopDamp;
	float turnBrake;
};