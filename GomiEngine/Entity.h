#pragma once

struct Entity
{
	unsigned int id;
	unsigned int generation;

	inline bool operator ==(const Entity& other) const
	{
		return id == other.id && generation == other.generation;
	}
	inline bool operator !=(const Entity& other) const
	{
		return id != other.id || generation != other.generation;
	}
};