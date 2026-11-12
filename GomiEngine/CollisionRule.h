#pragma once
#include "EnumNames.h"
#include "EntityTagComponent.h"
#include <unordered_map>
#include <string>

using RuleID = std::string;
enum class CollisionKind : uint8_t
{
	BLOCK,
	DAMAGE,
	TRIGGER
};
GOMI_ENUM_NAMES(CollisionKind,
	GOMI_ENUM_ENTRY(CollisionKind, BLOCK),
	GOMI_ENUM_ENTRY(CollisionKind, DAMAGE),
	GOMI_ENUM_ENTRY(CollisionKind, TRIGGER))

struct CollisionRule
{
	EntityTag a;
	EntityTag b;
	CollisionKind kind;

	inline const EntityTag getPair(EntityTag self) { return a == self ? b : a; }
};

// 当たり判定のルールの表(DLL をまたいで使えるよう、変数でなく関数にしている)
const std::unordered_map<RuleID, CollisionRule>& collisionRules();