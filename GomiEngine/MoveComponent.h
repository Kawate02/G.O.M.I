#pragma once
#include "Vec3.h"

struct MoveComponent
{
	Vec3 inputVel;
	Vec3 physVel;
	Vec3 result; //このフレームで動く量(位置の差)
	float speed;
	float maxSpeed;
	float stopDamp;
	float turnBrake;
};