#include "CollisionSystem.h"
#include "World.h"

void CollisionSystem::initialize(World& world)
{
	previous.clear();
}

void CollisionSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& cm = world.getComponentManager();
	auto& tags = cm.getComponentArray<EntityTagComponent>();

	//今重なっている組を集める(a を持つ物から b を持つ物を探す。a == b のルールや両方のタグを持つ物の重複は set で1つになる)
	std::set<Pair> current;
	for (auto& [id, rule] : CollisionRules::getInstance().all())
	{
		if (rule.kind != CollisionKind::TRIGGER) continue;
		cm.each<CollisionComponent, TransformComponent, EntityTagComponent>([&](Entity e) {
			if (!world.alive(e) || !tags.getComponent(e).has(rule.a)) return;
			for (auto& other : world.col().queryAt(world, e, rule.a, rule.b).entities)
			{
				current.insert(e < other ? Pair{ id, e, other } : Pair{ id, other, e });
			}
		});
	}

	//前のフレームと比べ、両方の Entity に出す(消えた側には出さない)
	auto send = [&](const Pair& p, CollisionPhase phase) {
		auto& [rule, lo, hi] = p;
		if (world.alive(lo)) world.event().emit(CollisionEvent{ lo, hi, rule, phase });
		if (world.alive(hi)) world.event().emit(CollisionEvent{ hi, lo, rule, phase });
	};
	auto cur = current.begin();
	auto prev = previous.begin();
	while (cur != current.end() || prev != previous.end())
	{
		if (prev == previous.end() || (cur != current.end() && *cur < *prev)) send(*cur++, CollisionPhase::ENTER);
		else if (cur == current.end() || *prev < *cur) send(*prev++, CollisionPhase::EXIT);
		else { send(*cur++, CollisionPhase::STAY); prev++; }
	}
	previous = std::move(current);
}
