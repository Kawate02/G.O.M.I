// 拾う物と痛い物: プレイヤー(PlayerControl を持つ物)が近づくと、PlayerStatus(Resource)の持ち物を増やす・HP を減らして消える
// 当たり判定のルールを使わず、Transform の位置の距離で見る簡単な例
#include "GameCode.h"
#include "PlayerStatus.h"
#include "TransformComponent.h"
#include "ControllerComponent.h"

#include <algorithm>

/// <summary>近づくと持ち物に item を count 個足して消える</summary>
struct PickupComponent
{
	std::string item = "gem";
	int count = 1;
	float radius = 48;
};

/// <summary>近づくと HP を damage 減らして消える</summary>
struct HazardComponent
{
	int damage = 1;
	float radius = 48;
};

class PickupSystem : public ISystem
{
private:
	static bool near(const Vec3& a, const Vec3& b, float radius)
	{
		float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
		return dx * dx + dy * dy + dz * dz <= radius * radius;
	}
public:
	void initialize(World&) override {}

	void update(World& world, int, float) override
	{
		auto& cm = world.getComponentManager();
		auto& status = world.resource<PlayerStatus>();
		cm.each<PlayerControlComponent, TransformComponent>([&](Entity player) {
			Vec3 p = cm.getComponentArray<TransformComponent>().getComponent(player).position;
			cm.each<PickupComponent, TransformComponent>([&](Entity e) {
				if (!world.alive(e)) return;
				auto& pickup = cm.getComponentArray<PickupComponent>().getComponent(e);
				if (!near(p, cm.getComponentArray<TransformComponent>().getComponent(e).position, pickup.radius)) return;
				status.items[pickup.item] += pickup.count;
				world.destroyEntity(e);
			});
			cm.each<HazardComponent, TransformComponent>([&](Entity e) {
				if (!world.alive(e)) return;
				auto& hazard = cm.getComponentArray<HazardComponent>().getComponent(e);
				if (!near(p, cm.getComponentArray<TransformComponent>().getComponent(e).position, hazard.radius)) return;
				status.hp = std::max(0, status.hp - hazard.damage);
				world.destroyEntity(e);
			});
		});
	}
};

GOMI_REGISTER
{
	components.add<PickupComponent>("Pickup")
		.field("item", &PickupComponent::item)
		.field("count", &PickupComponent::count)
		.field("radius", &PickupComponent::radius);
	components.add<HazardComponent>("Hazard")
		.field("damage", &HazardComponent::damage)
		.field("radius", &HazardComponent::radius);
	systems.add<PickupSystem>("PickupSystem");
}
