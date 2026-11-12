#include "CollisionRule.h"

const std::unordered_map<RuleID, CollisionRule>& collisionRules()
{
	static const std::unordered_map<RuleID, CollisionRule> table = {
		{ "unit_to_wall", { EntityTag::UNIT, EntityTag::WALL, CollisionKind::BLOCK }},
		{ "unit_to_unit", { EntityTag::UNIT, EntityTag::UNIT, CollisionKind::BLOCK }}
	};
	return table;
}
