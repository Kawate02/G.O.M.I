#pragma once
#include "EnumNames.h"
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
GOMI_ENUM_NAMES(ActionID,
	GOMI_ENUM_ENTRY(ActionID, MOVE_LEFT),
	GOMI_ENUM_ENTRY(ActionID, MOVE_RIGHT),
	GOMI_ENUM_ENTRY(ActionID, MOVE_UP),
	GOMI_ENUM_ENTRY(ActionID, MOVE_DOWN),
	GOMI_ENUM_ENTRY(ActionID, ATTACK),
	GOMI_ENUM_ENTRY(ActionID, JUMP),
	GOMI_ENUM_ENTRY(ActionID, Action),
	GOMI_ENUM_ENTRY(ActionID, DASH))
