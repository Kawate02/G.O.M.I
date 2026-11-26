#pragma once

#include <map>
#include <string>

/// <summary>
/// コマ送りで絵を動かす。クリップ(assets/animations/*.json)の名前とパスの表を持ち、今どれを流しているかと経った秒だけを覚える。
/// 今のコマは time から計算する(別に持たない)。SpriteAnimationSystem が Renderer の path と uv に書く
/// </summary>
struct SpriteAnimationComponent
{
	std::map<std::string, std::string> clips; //名前 → クリップのパス(例: "walk": "animations/bot_walk.json")
	std::string clip;                         //今流している名前(空なら何もしない)
	float time = 0;                           //クリップの頭から経った秒
	float speed = 1;                          //再生の速さ(2 で倍速)
	bool playing = true;
};
