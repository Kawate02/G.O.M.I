#include "ForwardMoveSystem.h"
#include "World.h"
#include "MathUtil.h"

#include <cmath>

float ForwardMoveSystem::WrapPi(float angle)
{
	if (angle > PI) return angle - 2 * PI;
	if (angle < -PI) return angle + 2 * PI;
	return angle;
}

float ForwardMoveSystem::FacingForward(float current, float target, float maxDelta)
{
	float delta = WrapPi(target - current);
	if (delta > maxDelta) delta = maxDelta;
	if (delta < -maxDelta) delta = -maxDelta;
	return current + delta;
}

void ForwardMoveSystem::initialize(World& world)
{
}

void ForwardMoveSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& forward = world.getComponentManager().getComponentArray<ForwardMoveComponent>();
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>();
	auto& move = world.getComponentManager().getComponentArray<MoveComponent>();

	world.getComponentManager().each<ForwardMoveComponent, TransformComponent, MoveComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tfc = tf.getComponent(e);
		auto& forwardc = forward.getComponent(e);
		auto& movec = move.getComponent(e);
		if (!forwardc.facing) return;

		float threshold = movec.maxSpeed * 0.01f;
		float thresholdSq = threshold * threshold;

		Vec2 vel = movec.result;
		if (vel.lengthSq() > thresholdSq)
		{
			float target = atan2(vel.y, vel.x);

			if (forwardc.turnSpeed <= 0)
			{
				tfc.rotation.z = MathUtil::toDegrees(target);
			}
			else
			{
				tfc.rotation.z = MathUtil::toDegrees(FacingForward(MathUtil::toRadians(tfc.rotation.z), target, forwardc.turnSpeed * deltaTime));
			}
		}
		});

	world.getComponentManager().each<ForwardMoveComponent, TransformComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tfc = tf.getComponent(e);
		auto& forwardc = forward.getComponent(e);
		if (!forwardc.enable) return;

		Vec2 vel = { 1, 0 };
		if (forwardc.additive) world.event().emit<MoveRequestEvent>({ .e = e, .velocity = vel.fromAngle(MathUtil::toRadians(tfc.rotation.z)) * forwardc.speed, .priority = 10, .blend = MoveBlend::ADDITIVE });
		else world.event().emit<MoveRequestEvent>({ .e = e, .velocity = vel.fromAngle(MathUtil::toRadians(tfc.rotation.z)), .priority = 10, .blend = MoveBlend::DIRECTIONAL});
		});
}
