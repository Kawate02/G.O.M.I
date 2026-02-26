#include "Prefab.h"
#include "World.h"

const std::unordered_map<PrefabID, PrefabDef> prefabTable = {
	{"", {}},
	{"Player", { "Player", Prefab_Player }},
	{"Player_Dummy", { "Player_Dummy", Prefab_Player_Dummy }},
	{"BackGround", { "BackGround", Prefab_BackGround }},
};