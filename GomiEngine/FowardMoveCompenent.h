#pragma once

struct FowardMoveComponent
{
	float speed;
	bool enable = false;
	bool facing = false;
	float turnSpeed;
	bool additive = false;
	bool pulse = false;
};