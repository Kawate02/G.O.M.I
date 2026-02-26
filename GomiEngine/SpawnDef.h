#pragma once
#include "Vec3.h"
#include <string>

using PrefabID = std::string;

struct SpawnDef
{
	PrefabID prefab;
	Vec3 pos;
	Vec3 scale;
	float rot;
};