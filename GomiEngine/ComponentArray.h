#pragma once

#include "Entity.h"
#include "Const.h"

template <typename T>
class ComponentArray
{
private:
	std::vector<uint32_t> generations;
	std::vector<int> entityToIndex;
	std::vector<int> indexToEntity;
	std::vector<T> components;
	int count = 0;
public:
	ComponentArray()
	{
		entityToIndex.resize(MAX_ENTITIES, -1);
	}
	inline void addComponent(Entity e, const T& component)
	{
		if (entityToIndex.size() > e.id && indexToEntity.size() > entityToIndex[e.id] && indexToEntity[entityToIndex[e.id]] == e.id && e.generation != generations[entityToIndex[e.id]])
		{
			components[entityToIndex[e.id]] = component;
			generations[entityToIndex[e.id]] = e.generation;
		}
		else
		{
			components.push_back(component);
			entityToIndex.resize(e.id + 1, -1);
			entityToIndex[e.id] = count;
			generations.push_back(e.generation);
			indexToEntity.push_back(e.id);
			count++;
		}
	}
	inline void removeComponent(Entity e)
	{
		int lastIndex = components.size() - 1;
		std::swap(components[lastIndex], components[entityToIndex[e.id]]);
		std::swap(indexToEntity[lastIndex], indexToEntity[entityToIndex[e.id]]);
		entityToIndex[indexToEntity[lastIndex]] = entityToIndex[e.id];

		components.pop_back();
		indexToEntity.pop_back();
		entityToIndex[e.id] = -1;
		count--;
	}
	inline bool hasComponent(Entity e) 
	{ 
		if (entityToIndex.size() <= e.id || entityToIndex[e.id] == -1
			|| generations.size() <= entityToIndex[e.id]) return false;
		return e.generation == generations[entityToIndex[e.id]]; 
	}
	inline T& getComponent(Entity e)
	{
		if (!hasComponent(e)) return components[0];
		return components[entityToIndex[e.id]];
	}
	inline const int getEntityId(int index) const
	{
		return indexToEntity[index];
	}
	inline const int getGeneration(int index) const
	{
		return generations[index];
	}
	inline int getComponentCount() const
	{
		return count;
	}
	inline void releaseAll()
	{
		components.clear();
		entityToIndex.clear();
		indexToEntity.clear();
		generations.clear();
		entityToIndex.resize(MAX_ENTITIES, -1);
		count = 0;
	}
};

