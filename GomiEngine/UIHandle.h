#pragma once
#include <cstdint>
struct UIHandle
{
	uint32_t id;
	uint8_t generation;

	inline bool operator==(const UIHandle& other) const { return id == other.id && generation == other.generation; }
};