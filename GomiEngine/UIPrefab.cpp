#include "UIPrefab.h"

const std::unordered_map<UIPrefabID, UIPrefabDef> uiPrefabTable = {
	{ "", {} },
	{ "Pause", { "Pause", UIPrefab_Pause }},
	{ "Title", { "Title", UIPrefab_Title }}	,
	{ "Result", { "Result", UIPrefab_Result }}
};