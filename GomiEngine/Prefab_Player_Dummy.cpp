#include "Prefab_Player_Dummy.h"
#include "ComponentType.h"

const PrefabDefinition Prefab_Player_Dummy = {
	{
		{
			{
				Component(TransformComponent({ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 0 })),
				Component(RendererComponent({ "player", 0, 0, true })),
				Component<MoveComponent>({ .maxSpeed = 0.2f, .stopDamp = 0.5f }),
				Component<EntityTagComponent>({ EntityTag::WALL | EntityTag::PLAYER }),
				Component<CollisionComponent>({ .type = CollisionType::BOX, .size = { 64, 64 } }),
			}
		}
	}
};
