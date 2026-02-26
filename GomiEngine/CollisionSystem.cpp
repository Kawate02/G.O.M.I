#include "CollisionSystem.h"
#include "World.h"

void CollisionSystem::initialize(World& world)
{
}

void CollisionSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& entities = world.getEntities();
	auto& collisions = world.getComponentManager().getComponentArray<CollisionComponent>();
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>();
	auto& tag = world.getComponentManager().getComponentArray<EntityTagComponent>();


	world.getComponentManager().each<CollisionComponent, TransformComponent, EntityTagComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tfc = tf.getComponent(e);
		auto& collc = collisions.getComponent(e);
		auto& tagc = tag.getComponent(e);
	});
}
