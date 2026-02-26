#include "PhysicsSystem.h"
#include "World.h"

void PhysicsSystem::initialize(World& world)
{
}

void PhysicsSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& physParam = world.getComponentManager().getComponentArray<PhysicsComponent>();
	auto& move = world.getComponentManager().getComponentArray<MoveComponent>();
	auto& moveEvents = world.event().getQueue<MoveRequestEvent>();

	world.getComponentManager().each<PhysicsComponent, MoveComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& movec = move.getComponent(e);
		auto& physc = physParam.getComponent(e);
		physc.isGround = false;
		auto it = moveEvents.find(e);
		if (it != moveEvents.end())
		{

			for (auto& ev : it->second)
			{
				if (ev.blend == MoveBlend::GRAVITY)
				{
					physc.gravVel = ev.velocity;
				}
				else if (ev.blend == MoveBlend::PULSE)
				{
					physc.extraVel += ev.velocity;
				}
			}
		}
		float gravity = physc.gravity;
		if (physc.gravVel.y < 0)
		{
			gravity *= 0.6f;
		}
		else gravity *= 1.2f;
		gravity = gravity > physc.maxGravity ? physc.maxGravity : gravity;
		physc.gravVel.y += gravity * deltaTime;
		
		float len = physc.extraVel.length();
		if (len > 0.0f)
		{
			float reduce = physc.decay * deltaTime;
			float newLen = max(0, len - reduce);
			physc.extraVel *= (newLen / len);
		}

		movec.physVel = physc.gravVel + physc.extraVel;
		auto hitE = world.col().queryAt(world, e, EntityTag::UNIT, ruleTable.at("unit_to_wall"), { movec.physVel.x, 0 });
		if (hitE.entities.size() > 0)
		{
			movec.physVel.x = 0;
		}

		hitE = world.col().queryAt(world, e, EntityTag::UNIT, ruleTable.at("unit_to_wall"), { 0, movec.physVel.y });
		if (hitE.entities.size() > 0)
		{
			if (movec.physVel.y >= 0 && physc.gravity > 0)
			{
				physc.isGround = true;
			}
			movec.physVel.y = 0;
		}
		});
}
