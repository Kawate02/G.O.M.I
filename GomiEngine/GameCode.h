#pragma once
// ゲームのコード(リポジトリの game/、GomiGame.dll になる)から include するヘッダ。
// Component・System を書き、同じファイルに GOMI_REGISTER で登録する:
//
//   struct HpComponent { int hp = 3; };
//   class DamageSystem : public ISystem { ... };
//
//   GOMI_REGISTER
//   {
//       components.add<HpComponent>("Hp").field("hp", &HpComponent::hp);
//       systems.add<DamageSystem>("DamageSystem");
//       resources.add<PlayerStatus>("PlayerStatus").field("hp", &PlayerStatus::hp);
//   }
//
// Resource(上の PlayerStatus)はシーンをまたいで残るデータ。System からは world.resource<PlayerStatus>() で読み書きする。
// 初期値は assets/resources.json に { "PlayerStatus": { "hp": 10 } } のように書く
//
// エディタは game/ の保存を見てビルドし、開いたまま読み直す。ゲームの exe は起動時に読む
#include "ComponentRegistry.h"
#include "SystemRegistry.h"
#include "ResourceRegistry.h"
#include "ISystem.h"
#include "World.h"

#include <vector>

using GameRegisterFunc = void(*)(ComponentRegistry& components, SystemRegistry& systems);

/// <summary>
/// この DLL の中の登録の塊の一覧(実体は game_entry/GameEntry.cpp。DLL ごとに1つ)
/// </summary>
std::vector<GameRegisterFunc>& gomiGameRegistrations();

#define GOMI_CONCAT_IMPL(a, b) a##b
#define GOMI_CONCAT(a, b) GOMI_CONCAT_IMPL(a, b)
#define GOMI_REGISTER_AT(id) \
	static void GOMI_CONCAT(gomiRegisterBody_, id)(ComponentRegistry& components, SystemRegistry& systems, ResourceRegistry& resources); \
	static void GOMI_CONCAT(gomiRegister_, id)(ComponentRegistry& c, SystemRegistry& s) { GOMI_CONCAT(gomiRegisterBody_, id)(c, s, ResourceRegistry::getInstance()); } \
	static const bool GOMI_CONCAT(gomiRegistered_, id) = (gomiGameRegistrations().push_back(&GOMI_CONCAT(gomiRegister_, id)), true); \
	static void GOMI_CONCAT(gomiRegisterBody_, id)([[maybe_unused]] ComponentRegistry& components, [[maybe_unused]] SystemRegistry& systems, [[maybe_unused]] ResourceRegistry& resources)
/// <summary>
/// 続けて { } の中に components.add<...>(...) / systems.add<...>(...) / resources.add<...>(...) を書く。DLL を読んだ時に流れる
/// </summary>
#define GOMI_REGISTER GOMI_REGISTER_AT(__COUNTER__)
