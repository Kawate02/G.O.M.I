#include "Stage_Field.h"
#include "Prefab_Player.h"
#include "Prefab_Player_Dummy.h"
#include "Prefab_BackGround.h"

const StageDefinition Stage_Field = {
	"Field",
	{
		System<PlayerControlSystem>(),
		System<FowardMoveSystem>(),
		System<PhysicsSystem>(),
		System<MoveResolveSystem>(),
		System<MoveSystem>(),
		System<CameraSystem>(),
		System<OffScreenSystem>(),
		System<RendererSystem>(),
		System<CollisionSystem>(),
		System<BackGroundSystem>()
	},
	{
		{ &Prefab_Player, Vec3{ 100.0f, 300.0f, 0.0f } },
		{ &Prefab_BackGround },
	},
	{

	}
};
