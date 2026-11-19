#pragma once
#include "Entity.h"
#include "CollisionRule.h"

/// <summary>
/// ENTER: このフレームで重なり始めた。STAY: 前のフレームから重なっている(毎フレーム出る)。
/// EXIT: 離れた、または相手が消えた(消えた側には出ない)
/// </summary>
enum class CollisionPhase : uint8_t
{
	ENTER,
	STAY,
	EXIT
};
GOMI_ENUM_NAMES(CollisionPhase,
	GOMI_ENUM_ENTRY(CollisionPhase, ENTER),
	GOMI_ENUM_ENTRY(CollisionPhase, STAY),
	GOMI_ENUM_ENTRY(CollisionPhase, EXIT))

/// <summary>
/// TRIGGER のルールに合う重なり。CollisionSystem が重なった両方の Entity に1つずつ出す(self が受け取る側)。
/// 受け取り方: world.event().collisions()(出た順、毎回同じ)か world.event().collisionsOf(e)。フレームの終わりに消えるので、
/// 受ける System はシーンの systems で CollisionSystem より後に置く
/// </summary>
struct CollisionEvent
{
	Entity self;
	Entity other;
	RuleID rule;
	CollisionPhase phase = CollisionPhase::ENTER;
};
