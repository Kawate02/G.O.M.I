#pragma once
#include "Vec2.h"
#include <string>

using UIPrefabID = std::string;

struct UISpawnDef
{
	UIPrefabID prefab;
	Vec2 pos;
	int layer = 0;
};