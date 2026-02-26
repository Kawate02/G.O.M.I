#pragma once
#include <cstdint>

enum class ActionID : uint8_t
{
	MOVE_LEFT,
	MOVE_RIGHT,
	MOVE_UP,
	MOVE_DOWN,
	ATTACK,
	JUMP,
	Action,
	DASH
};