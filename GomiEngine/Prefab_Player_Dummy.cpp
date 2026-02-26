#include "Prefab.h"
#include "World.h"

Entity Prefab_Player_Dummy(World& world, const SpawnDef& def)
{
	Entity e = world.createEntity();
	auto& component = world.getComponentManager();
	component.addComponent(e, TransformComponent({ def.pos, def.scale, def.rot }));
	component.addComponent(e, RendererComponent({ "player", 0, 0, true }));
	component.addComponent<MoveComponent>(e, { .maxSpeed = 0.2f, .stopDamp = 0.5f });
	component.addComponent<EntityTagComponent>(e, { EntityTag::WALL | EntityTag::PLAYER });
	component.addComponent<CollisionComponent>(e, { .type = CollisionType::BOX, .size = { 64, 64 } });

	return e;
}