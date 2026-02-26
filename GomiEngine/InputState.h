#pragma once
#include "KeyCode.h"
#include "Vec2.h"
#include "Mouse.h"

struct InputState
{
	char key[256] = { 0 };
	char deltakey[256] = { 0 };
	Mouse mouse;
	inline bool isPressedKey(InputKey key) const
	{
		return this->key[static_cast<int>(key)] != 0;
	}
	inline bool isDownKey(InputKey key) const
	{
		return this->key[static_cast<int>(key)] != 0 && this->deltakey[static_cast<int>(key)] == 0;
	}
	inline bool isUpKey(InputKey key) const
	{
		return this->key[static_cast<int>(key)] == 0 && this->deltakey[static_cast<int>(key)] != 0;
	}
};