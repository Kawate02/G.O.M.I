#pragma once
#include "SpawnDef.h"
#include "UISpawnDef.h"
#include "SystemType.h"
#include <vector>
#include <string>
#include <unordered_map>

using StageID = int;

struct StageDef
{
	int id;
	std::vector<SystemType> systems;
	std::vector<SpawnDef> prefabs;
	std::vector<UISpawnDef> uiPrefabs;
};

const extern std::unordered_map<StageID, StageDef> stageTable;