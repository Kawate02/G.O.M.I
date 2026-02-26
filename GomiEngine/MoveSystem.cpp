#include "MoveSystem.h"
#include "World.h"

void MoveSystem::initialize(World& world)
{
}

void MoveSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& move = world.getComponentManager().getComponentArray<MoveComponent>();
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>();

	world.getComponentManager().each<MoveComponent, TransformComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tfc = tf.getComponent(e);
		auto& movec = move.getComponent(e);
		tfc.position.x += movec.result.x;
		tfc.position.y += movec.result.y;
		});
}
