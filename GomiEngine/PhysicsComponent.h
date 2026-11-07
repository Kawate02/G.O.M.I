#pragma once
#include "Vec3.h"

struct PhysicsComponent
{
	Vec3 gravVel;
	Vec3 extraVel;
	Vec3 gravityDirection = { 0, 1, 0 }; //重力の向き。2D(画面座標)は下が +y、3D(Y が上)は {0, -1, 0}
	float gravity = 2400;
	float maxGravity = 3200;
	float decay = 3000;
	bool isGround = true;
};