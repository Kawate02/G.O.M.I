#pragma once
#include "Entity.h"
#include "Vec2.h"
enum class MoveBlend : uint8_t
{
	DIRECTIONAL,
	ADDITIVE,
	GRAVITY,
	PULSE
};
enum class MoveOwner : uint8_t
{
	PLAYER,
	SYSTEM
};

struct MoveRequestEvent
{
	Entity e;
	Vec2 velocity;
	int priority;
	MoveBlend blend;
	MoveOwner owner;
};