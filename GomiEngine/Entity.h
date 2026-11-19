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
	/// <summary>
	/// ID → 世代の順(毎回同じ順で並べるため)
	/// </summary>
	inline bool operator <(const Entity& other) const
	{
		return id != other.id ? id < other.id : generation < other.generation;
	}
};