#pragma once

#include "ISystem.h"
#include "RendererSystem.h"
#include "PlayerControlSystem.h"
#include "MoveSystem.h"
#include "PhysicsSystem.h"
#include "MoveResolveSystem.h"
#include "FowardMoveSystem.h"
#include "CameraSystem.h"
#include "CollisionSystem.h"
#include "BackGroundSystem.h"
#include "OffScreenSystem.h"

#include "SystemType.h"

std::unique_ptr<ISystem> createSystem(SystemType type);