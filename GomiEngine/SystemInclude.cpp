#include "SystemInclude.h"

std::unique_ptr<ISystem> createSystem(SystemType type)
{
	switch (type)
	{
	case SystemType::PlayerControlSystem:
		return std::make_unique<PlayerControlSystem>();
	case SystemType::MoveSystem:
		return std::make_unique<MoveSystem>();
	case SystemType::MoveResolveSystem:
		return std::make_unique<MoveResolveSystem>();
	case SystemType::PhysicsSystem:
		return std::make_unique<PhysicsSystem>();
	case SystemType::CameraSystem:
		return std::make_unique<CameraSystem>();
	case SystemType::RendererSystem:
		return std::make_unique<RendererSystem>();
	case SystemType::CollisionSystem:
		return std::make_unique<CollisionSystem>();
	case SystemType::FowardMoveSystem:
		return std::make_unique<FowardMoveSystem>();
	case SystemType::BackGroundSystem:
		return std::make_unique<BackGroundSystem>();
	case SystemType::OffScreenSystem:
		return std::make_unique<OffScreenSystem>();
	default:
		return nullptr;
	}
}
