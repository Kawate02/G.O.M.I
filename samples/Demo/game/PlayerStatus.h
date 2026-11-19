#pragma once
// PlayerStatus: プレイヤーの HP と持ち物。シーンを切り替えても残る(Resource)
// System からは world.resource<PlayerStatus>() で読み書きする。初期値は assets/resources.json(エディタの Resource パネル)
#include "JsonConvert.h"

#include <map>
#include <string>

struct PlayerStatus
{
	int hp = 3;
	std::map<std::string, int> items; //アイテムの名前 → 個数
};
