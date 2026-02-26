#include "FowardMoveSystem.h"
#include "World.h"

#include <cmath>

float FowardMoveSystem::WrapPi(float angle)
{
	if (angle > PI) return angle - 2 * PI;
	if (angle < -PI) return angle + 2 * PI;
	return angle;
}

float FowardMoveSystem::FacingForward(float current, float target, float maxDelta)
{
	float delta = WrapPi(target - current);
	if (delta > maxDelta) delta = maxDelta;
	if (delta < -maxDelta) delta = -maxDelta;
	return current + delta;
}

void FowardMoveSystem::initialize(World& world)
{
}

void FowardMoveSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& foward = world.getComponentManager().getComponentArray<FowardMoveComponent>();
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>();
	auto& move = world.getComponentManager().getComponentArray<MoveComponent>();

	world.getComponentManager().each<FowardMoveComponent, TransformComponent, MoveComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tfc = tf.getComponent(e);
		auto& fowardc = foward.getComponent(e);
		auto& movec = move.getComponent(e);
		if (!fowardc.facing) return;

		float threshold = movec.maxSpeed * 0.01f;
		float thresholdSq = threshold * threshold;

		Vec2 vel = movec.result;
		if (vel.lengthSq() > thresholdSq)
		{
			float target = atan2(vel.y, vel.x);

			if (fowardc.turnSpeed <= 0)
			{
				tfc.angle = target;
			}
			else
			{
				tfc.angle = FacingForward(tfc.angle, target, fowardc.turnSpeed * deltaTime);
			}
		}
		});

	world.getComponentManager().each<FowardMoveComponent, TransformComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tfc = tf.getComponent(e);
		auto& fowardc = foward.getComponent(e);
		if (!fowardc.enable) return;

		Vec2 vel = { 1, 0 };
		if (fowardc.additive) world.event().emit<MoveRequestEvent>({ .e = e, .velocity = vel.fromAngle(tfc.angle) * fowardc.speed, .priority = 10, .blend = MoveBlend::ADDITIVE });
		else world.event().emit<MoveRequestEvent>({ .e = e, .velocity = vel.fromAngle(tfc.angle), .priority = 10, .blend = MoveBlend::DIRECTIONAL});
		});
}
