#pragma once

#include <cstdint>

enum class EntityTag : uint32_t
{
	NONE = 0,
	UNIT = 1 << 0,
	PLAYER = 1 << 1,
	ENEMY = 1 << 2,
	WALL = 1 << 3,
	ITEM = 1 << 4,
	BULLET = 1 << 5,
	BOSS = 1 << 6
};
inline EntityTag operator|(EntityTag a, EntityTag b) { return static_cast<EntityTag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
inline EntityTag operator&(EntityTag a, EntityTag b) { return static_cast<EntityTag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); }

inline bool hasTag(EntityTag a, EntityTag b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }