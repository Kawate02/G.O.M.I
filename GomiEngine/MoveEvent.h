#pragma once
#include "Entity.h"
#include "Vec3.h"
#include <cstdint>
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
	Vec3 velocity;
	int priority;
	MoveBlend blend;
	MoveOwner owner;
};