#pragma once
#include "UIBase.h"
#include "UISpawnDef.h"
#include <vector>
#include <memory>

class UIManager;
using UIPrefabFunc = UIHandle(*)(UIManager& manager, const UISpawnDef& def);

struct UIPrefabDef
{
	UIPrefabID prefabID;
	UIPrefabFunc func;
};

const extern std::unordered_map<UIPrefabID, UIPrefabDef> uiPrefabTable;

UIHandle UIPrefab_Pause(UIManager& manager, const UISpawnDef& def);
UIHandle UIPrefab_Title(UIManager& manager, const UISpawnDef& def);
UIHandle UIPrefab_Result(UIManager& manager, const UISpawnDef& def);