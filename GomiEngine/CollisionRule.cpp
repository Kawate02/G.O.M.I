#include "CollisionRule.h"

const std::unordered_map<RuleID, CollisionRule> ruleTable = {
	{ "unit_to_wall", { EntityTag::UNIT, EntityTag::WALL, CollisionKind::BLOCK }},
	{ "unit_to_unit", { EntityTag::UNIT, EntityTag::UNIT, CollisionKind::BLOCK }}
};