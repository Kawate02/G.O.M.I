#include "PlayerControlSystem.h"
#include "World.h"

void PlayerControlSystem::initialize(World& world)
{
}

void PlayerControlSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& input = world.getInputState();
	auto& player = world.getComponentManager().getComponentArray<PlayerControlComponent>();
	auto& physics = world.getComponentManager().getComponentArray<PhysicsComponent>();

	world.getComponentManager().each<PlayerControlComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& pc = player.getComponent(e);
		auto pressed = [&](ActionID a) { auto it = pc.actions.find(a); return it != pc.actions.end() && input.isPressedKey(it->second); };
		auto down = [&](ActionID a) { auto it = pc.actions.find(a); return it != pc.actions.end() && input.isDownKey(it->second); };

		//上下の入力: 2D は画面の上(-y)、3D の地面は奥(-z)
		Vec3 up = pc.plane == MovePlane::XZ ? Vec3(0, 0, -1) : Vec3(0, -1, 0);
		auto move = [&](const Vec3& dir) {
			world.event().emit(MoveRequestEvent{ e, dir, 10, MoveBlend::DIRECTIONAL, MoveOwner::PLAYER });
		};
		if (pressed(ActionID::MOVE_RIGHT)) move(Vec3(1, 0, 0));
		if (pressed(ActionID::MOVE_LEFT)) move(Vec3(-1, 0, 0));
		if (pressed(ActionID::MOVE_UP)) move(up);
		if (pressed(ActionID::MOVE_DOWN)) move(-up);

		if (down(ActionID::JUMP) && physics.hasComponent(e))
		{
			auto& physc = physics.getComponent(e);
			if (physc.isGround)
			{
				world.event().emit(MoveRequestEvent{ e, -physc.gravityDirection * pc.jumpSpeed, 10, MoveBlend::GRAVITY, MoveOwner::PLAYER });
			}
		}
		if (down(ActionID::DASH))
		{
			world.event().emit(MoveRequestEvent{ e, Vec3(pc.dashSpeed, 0, 0), 10, MoveBlend::PULSE, MoveOwner::PLAYER });
		}
		});
}
