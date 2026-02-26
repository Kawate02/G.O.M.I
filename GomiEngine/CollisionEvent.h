#pragma once
#include "Entity.h"
#include "CollisionRule.h"

struct CollisionEvent
{
	Entity self;
	Entity other;
	CollisionRule rule;
};