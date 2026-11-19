// 拾う物と痛い物: プレイヤーが触れると、PlayerStatus(Resource)の持ち物を増やす・HP を減らして消える。
// 触れたかどうかは当たり判定のルール(assets/collision_rules.json の TRIGGER)で決まり、CollisionSystem がイベントを出す。
// ここではそのイベントを受けて何をするかだけを書く(シーンの systems で CollisionSystem より後に置く)
#include "GameCode.h"
#include "PlayerStatus.h"

#include <algorithm>

/// <summary>触れると持ち物に item を count 個足して消える(タグ ITEM と Collision も付ける)</summary>
struct PickupComponent
{
	std::string item = "gem";
	int count = 1;
};

/// <summary>触れると HP を damage 減らして消える(タグ HAZARD と Collision も付ける)</summary>
struct HazardComponent
{
	int damage = 1;
};

class PickupSystem : public ISystem
{
public:
	void initialize(World&) override {}

	void update(World& world, int, float) override
	{
		auto& cm = world.getComponentManager();
		auto& pickups = cm.getComponentArray<PickupComponent>();
		auto& hazards = cm.getComponentArray<HazardComponent>();
		auto& status = world.resource<PlayerStatus>();
		//このフレームの当たり判定のイベント(出た順。毎回同じ)。self が受け取った側
		for (auto& ev : world.event().collisions())
		{
			if (ev.phase != CollisionPhase::ENTER || !world.alive(ev.self)) continue;
			if (ev.rule == "player_item" && pickups.hasComponent(ev.self))
			{
				auto& pickup = pickups.getComponent(ev.self);
				status.items[pickup.item] += pickup.count;
				world.destroyEntity(ev.self);
			}
			else if (ev.rule == "player_hazard" && hazards.hasComponent(ev.self))
			{
				status.hp = std::max(0, status.hp - hazards.getComponent(ev.self).damage);
				world.destroyEntity(ev.self);
			}
		}
	}
};

GOMI_REGISTER
{
	components.add<PickupComponent>("Pickup")
		.field("item", &PickupComponent::item)
		.field("count", &PickupComponent::count);
	components.add<HazardComponent>("Hazard")
		.field("damage", &HazardComponent::damage);
	systems.add<PickupSystem>("PickupSystem");
}
