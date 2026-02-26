#include "CameraSystem.h"
#include "World.h"

void CameraSystem::initialize(World& world)
{
}

void CameraSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& fEntity = world.getComponentManager().getComponentArray<CameraFollowComponent>();
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>();
	auto& camera = world.getCamera();

	world.getComponentManager().each<CameraFollowComponent, TransformComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tfc = tf.getComponent(e);
		auto& fEntityc = fEntity.getComponent(e);

		if (fEntityc.x) camera.position.x = tfc.position.x + fEntityc.offset.x;
		if (fEntityc.y) camera.position.y = tfc.position.y + fEntityc.offset.y;
		});
}
