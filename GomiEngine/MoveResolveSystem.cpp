#include "MoveResolveSystem.h"
#include "World.h"

#include <algorithm>

void MoveResolveSystem::initialize(World& world)
{
}

void MoveResolveSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& move = world.getComponentManager().getComponentArray<MoveComponent>();
	auto& moveEvents = world.event().getQueue<MoveRequestEvent>();

	world.getComponentManager().each<MoveComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto it = moveEvents.find(e);
		auto& movec = move.getComponent(e);
		if (!(it != moveEvents.end() && std::any_of(it->second.begin(), it->second.end(),
			[](const auto& s) { return s.owner == MoveOwner::PLAYER; })))
		{
			movec.inputVel *= movec.stopDamp;
		}

		if (it != moveEvents.end())
		{
			Vec2 finVec;
			Vec2 addVec;
			int bestPriority = -1;

			for (auto& ev : it->second)
			{
				if (ev.blend == MoveBlend::DIRECTIONAL)
				{
					if (ev.priority > bestPriority)
					{
						finVec = ev.velocity;
						bestPriority = ev.priority;
					}
					else if (ev.priority == bestPriority)
					{
						finVec += ev.velocity;
					}
				}
				else if (ev.blend == MoveBlend::ADDITIVE)
				{
					addVec += ev.velocity;
				}
			}
			finVec = finVec.normalize();

			if (movec.inputVel.length() > 0)
			{
				Vec2 parallel = finVec * movec.inputVel.dot(finVec);
				Vec2 perpendicular = movec.inputVel - parallel;
				movec.inputVel = parallel + perpendicular * movec.turnBrake;
			}
			movec.inputVel += finVec * movec.speed;
			float lenght = movec.inputVel.x * movec.inputVel.x + movec.inputVel.y * movec.inputVel.y;
			if (lenght > movec.maxSpeed * movec.maxSpeed)
			{
				movec.inputVel = movec.inputVel.normalize() * movec.maxSpeed;
			}

			movec.inputVel += addVec;
		}

		movec.result = movec.inputVel * deltaTime + movec.physVel * deltaTime;
		auto hitE = world.col().queryAt(world, e, EntityTag::UNIT, ruleTable.at("unit_to_wall"), { movec.result.x, 0 });
		if (hitE.entities.size() > 0)
		{
			movec.result.x = 0;
		}

		hitE = world.col().queryAt(world, e, EntityTag::UNIT, ruleTable.at("unit_to_wall"), { 0, movec.result.y });
		if (hitE.entities.size() > 0)
		{
			movec.result.y = 0;
		}
		});
}
