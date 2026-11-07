#pragma once
#include "EnumNames.h"
#include "Vec2.h"

enum class CollisionType
{
	BOX,
	CIRCLE
};
GOMI_ENUM_NAMES(CollisionType,
	GOMI_ENUM_ENTRY(CollisionType, BOX),
	GOMI_ENUM_ENTRY(CollisionType, CIRCLE))


struct CollisionComponent
{
	CollisionType type = CollisionType::BOX;
	Vec2 size;
	float radius;
	Vec2 offset;
};