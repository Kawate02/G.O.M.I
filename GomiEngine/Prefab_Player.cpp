#include "Prefab_Player.h"
#include "ComponentType.h"

const PrefabDefinition Prefab_Player = {
	{
		{
			{
				Component(TransformComponent({ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 0 })),
				Component(RendererComponent({ "player", 0, 0, true })),
				Component<PlayerControlComponent>({
					{
						{ ActionID::MOVE_RIGHT, InputKey::KEY_D },
						{ ActionID::MOVE_LEFT, InputKey::KEY_A },
						{ ActionID::MOVE_UP, InputKey::KEY_W },
						{ ActionID::MOVE_DOWN, InputKey::KEY_S },
						{ ActionID::JUMP, InputKey::KEY_SPACE },
						{ ActionID::DASH, InputKey::KEY_LSHIFT },
					} }),
				Component<MoveComponent>({ .speed = 50, .maxSpeed = 400, .stopDamp = 0.5f, .turnBrake = 0.6f }),
				Component<CameraFollowComponent>({ .offset = { -100, -300 }, .y = false }),
				Component<EntityTagComponent>({ EntityTag::UNIT | EntityTag::PLAYER }),
				Component<CollisionComponent>({ .type = CollisionType::CIRCLE, .radius = 32, .offset = { 32, 32 } }),
				Component<PhysicsComponent>({ .gravity = 0 }),
				Component<FowardMoveComponent>({ .speed = 200, .enable = true, .additive = true }),
			}
		}
	}
};
