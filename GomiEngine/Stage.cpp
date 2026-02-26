#include "Stage.h"

const std::unordered_map<StageID, StageDef> stageTable = {
	{ 0, {
		0,
		{
			SystemType::RendererSystem,
		},
		{
			
		},
		{
			{ "Title", { 0.0f, 0.0f } }
		}
	}},
	{ 1, {
		1,
		{
			SystemType::PlayerControlSystem,
			SystemType::FowardMoveSystem,
			SystemType::PhysicsSystem,
			SystemType::MoveResolveSystem,
			SystemType::MoveSystem,
			SystemType::CameraSystem,
			SystemType::OffScreenSystem,
			SystemType::RendererSystem,
			SystemType::CollisionSystem,
			SystemType::BackGroundSystem
		},
		{
			{ "Player", { 100.0f, 300.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 0 },
			{ "Player_Dummy", { 100.0f, 400.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 0 },
			{ "BackGround", { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 0 },
		},
		{

		}
	}},
	{ 999, {
		999,
		{
			SystemType::RendererSystem,
		},
		{

		},
		{

		}
	}},
};