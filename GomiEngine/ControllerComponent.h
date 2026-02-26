#pragma once

#include <unordered_map>
#include "Action.h"
#include "KeyCode.h"

struct PlayerControlComponent
{
	std::unordered_map<ActionID, InputKey> actions;
};