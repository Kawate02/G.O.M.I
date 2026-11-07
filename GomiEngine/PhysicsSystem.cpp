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
		//上昇中(重力と逆向き)は弱く、落下中は強く引く
		const Vec3& down = physc.gravityDirection;
		float gravity = physc.gravity;
		if (physc.gravVel.dot(down) < 0)
		{
			gravity *= 0.6f;
		}
		else gravity *= 1.2f;
		gravity = gravity > physc.maxGravity ? physc.maxGravity : gravity;
		physc.gravVel += down * (gravity * deltaTime);

		float len = physc.extraVel.length();
		if (len > 0.0f)
		{
			float reduce = physc.decay * deltaTime;
			float newLen = std::max(0.0f, len - reduce);
			physc.extraVel *= (newLen / len);
		}

		movec.physVel = physc.gravVel + physc.extraVel;
		//このフレームで動く距離(速度 x 時間)の先に壁があるかを軸ごとに見る。重力の向きで止まったら接地
		for (int axis = 0; axis < 3; axis++)
		{
			Vec3 step;
			step[axis] = movec.physVel[axis] * deltaTime;
			auto hitE = world.col().queryAt(world, e, EntityTag::UNIT, ruleTable.at("unit_to_wall"), step);
			if (hitE.entities.size() > 0)
			{
				if (down[axis] != 0 && movec.physVel[axis] * down[axis] >= 0 && physc.gravity > 0)
				{
					physc.isGround = true;
				}
				movec.physVel[axis] = 0;
				//床や天井で止まったら落下の速さも捨てる(残すと段差から落ちた瞬間に一気に落ちる)
				if (down[axis] != 0) physc.gravVel[axis] = 0;
			}
		}
		});
}
