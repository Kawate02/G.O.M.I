#pragma once
#include "EntityTag.h"

struct EntityTagComponent
{
	EntityTag tags;
	inline bool has(EntityTag t) { return hasTag(tags, t); }
	inline void set(EntityTag t) { tags | t; }
};