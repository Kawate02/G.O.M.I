#pragma once
#include "Vec2.h"

enum class CollisionType
{
	BOX,
	CIRCLE
};

struct CollisionComponent
{
	CollisionType type = CollisionType::BOX;
	Vec2 size;
	float radius;
	Vec2 offset;
};