#pragma once
#include "ISystem.h"
#include "CollisionContext.h"

#include <set>
#include <tuple>

/// <summary>
/// TRIGGER のルール(assets/collision_rules.json)に合う重なりを毎フレーム探し、両方の Entity に CollisionEvent
/// (ENTER / STAY / EXIT)を出す。出す順はルールID → Entity の ID の順で毎回同じ
/// </summary>
class CollisionSystem : public ISystem
{
private:
	/// <summary>
	/// 重なっている組(ルールID, ID の小さい方, 大きい方)
	/// </summary>
	using Pair = std::tuple<RuleID, Entity, Entity>;
	std::set<Pair> previous; //前のフレームで重なっていた組
public:
	void initialize(World& world) override;
	void update(World& world, int frameCount, float deltaTime) override;
	Json runState(const std::vector<Entity>& order) const override;
	void restoreRunState(const Json& state, const std::vector<Entity>& order) override;
};
