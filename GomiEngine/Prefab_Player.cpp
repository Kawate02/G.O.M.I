#include "Prefab.h"
#include "World.h"

Entity Prefab_Player(World& world, const SpawnDef& def)
{
	Entity e = world.createEntity();
	auto& component = world.getComponentManager();
	component.addComponent(e, TransformComponent({ def.pos, def.scale, def.rot }));
	component.addComponent(e, RendererComponent({ "player", 0, 0, true }));
	component.addComponent<PlayerControlComponent>(e, {
		{
			{ ActionID::MOVE_RIGHT, InputKey::KEY_D },
			{ ActionID::MOVE_LEFT, InputKey::KEY_A },
			{ ActionID::MOVE_UP, InputKey::KEY_W },
			{ ActionID::MOVE_DOWN, InputKey::KEY_S },
			{ ActionID::JUMP, InputKey::KEY_SPACE },
			{ ActionID::DASH, InputKey::KEY_LSHIFT },
		} });
	component.addComponent<MoveComponent>(e, { .speed = 50, .maxSpeed = 400, .stopDamp = 0.5f, .turnBrake = 0.6f });
	component.addComponent<CameraFollowComponent>(e, { .offset = { -100, -300 }, .y = false });
	component.addComponent<EntityTagComponent>(e, { EntityTag::UNIT | EntityTag::PLAYER });
	component.addComponent<CollisionComponent>(e, { .type = CollisionType::CIRCLE, .radius = 32,.offset = { 32, 32 } });
	component.addComponent<PhysicsComponent>(e, { .gravity = 0 });
	component.addComponent<FowardMoveComponent>(e, { .speed = 200, .enable = true, .additive = true });

	return e;
}