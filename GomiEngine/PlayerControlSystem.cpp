#include "PlayerControlSystem.h"
#include "World.h"

void PlayerControlSystem::initialize(World& world)
{
}

void PlayerControlSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& input = world.getInputState();
	auto& entities = world.getEntities();
	auto& player = world.getComponentManager().getComponentArray<PlayerControlComponent>();

	world.getComponentManager().each<PlayerControlComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& pc = player.getComponent(e);
		if (input.isPressedKey(pc.actions[ActionID::MOVE_RIGHT]))
		{
			world.event().emit(MoveRequestEvent { e, Vec2(1, 0), 10, MoveBlend::DIRECTIONAL, MoveOwner::PLAYER  });
		}
		if (input.isPressedKey(pc.actions[ActionID::MOVE_LEFT]))
		{
			world.event().emit(MoveRequestEvent { e, Vec2(-1, 0), 10, MoveBlend::DIRECTIONAL, MoveOwner::PLAYER  });
		}
		if (input.isPressedKey(pc.actions[ActionID::MOVE_UP]))
		{
			world.event().emit(MoveRequestEvent { e, Vec2(0, -1), 10, MoveBlend::DIRECTIONAL, MoveOwner::PLAYER  });
		}
		if (input.isPressedKey(pc.actions[ActionID::MOVE_DOWN]))
		{
			world.event().emit(MoveRequestEvent { e, Vec2(0, 1), 10, MoveBlend::DIRECTIONAL, MoveOwner::PLAYER });
		}
		if (input.isDownKey(pc.actions[ActionID::JUMP]))
		{
			if (world.getComponentManager().has<PhysicsComponent>(e) && world.getComponentManager().getComponentArray<PhysicsComponent>().getComponent(e).isGround)
			world.event().emit(MoveRequestEvent { e, Vec2(0, -800), 10, MoveBlend::GRAVITY, MoveOwner::PLAYER });
		}
		if (input.isDownKey(pc.actions[ActionID::DASH]))
		{
			world.event().emit(MoveRequestEvent { e, Vec2(1000, 0), 10, MoveBlend::PULSE, MoveOwner::PLAYER });
		}
		});
}
