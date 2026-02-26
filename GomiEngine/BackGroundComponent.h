#pragma once
#include <string>
#include "Vec2.h"

struct BackGroundComponent
{
	std::string asset;
	Vec2 origin = { 0, 0 };
	Vec2 parallax = { 1, 1 };
	int layer = -100;
	int order = 0;
	bool enable = true;
	int margin = 0;
};