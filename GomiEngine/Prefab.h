#pragma once
#include "EntityHash.h"
#include "SpawnDef.h"

#include <unordered_map>

class World;
using PrefabFunc = Entity(*)(World& world, const SpawnDef& def);
using PrefabID = std::string;

struct PrefabDef
{
	PrefabID prefab;
	PrefabFunc func;
};

const extern std::unordered_map<PrefabID, PrefabDef> prefabTable;

Entity Prefab_Player(World& world, const SpawnDef& def);
Entity Prefab_Player_Dummy(World& world, const SpawnDef& def);
Entity Prefab_BackGround(World& world, const SpawnDef& def);