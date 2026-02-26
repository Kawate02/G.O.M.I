#pragma once
#include "Utilities.h"

struct Mouse 
{
	Vec2 position = { (int)0, (int)0 };
	int totalWheel = 0;

	Vec2 deltaPosition = { (int)0, (int)0 };
	int deltaWheel = 0;
};