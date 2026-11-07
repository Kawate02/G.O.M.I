#pragma once

#include <vector>
#include <cstdint>
#include "Entity.h"
#include "Const.h"

/// <summary>
/// Sparse Set による1種類のコンポーネントの格納先。
/// entityToIndex(Entity ID → 密配列の添字)と、密配列 components / indexToEntity / generations を持つ
/// </summary>
template <typename T>
class ComponentArray
{
private:
	std::vector<uint32_t> generations;
	std::vector<int> entityToIndex;
	std::vector<int> indexToEntity;
	std::vector<T> components;
	int count = 0;

	inline int indexOf(unsigned int id) const
	{
		return id < entityToIndex.size() ? entityToIndex[id] : -1;
	}
public:
	ComponentArray()
	{
		entityToIndex.resize(MAX_ENTITIES, -1);
	}
	/// <summary>
	/// 追加する。同じ ID の枠が既にあれば(古い世代の残りでも)上書きする
	/// </summary>
	inline void addComponent(Entity e, const T& component)
	{
		int index = indexOf(e.id);
		if (index != -1)
		{
			components[index] = component;
			generations[index] = e.generation;
			return;
		}
		if (e.id >= entityToIndex.size()) entityToIndex.resize(e.id + 1, -1);
		entityToIndex[e.id] = count;
		components.push_back(component);
		generations.push_back(e.generation);
		indexToEntity.push_back(e.id);
		count++;
	}
	/// <summary>
	/// 末尾と入れ替えて消す(O(1))。世代は見ないので、ID の枠ごと消える
	/// </summary>
	inline void removeComponent(Entity e)
	{
		int index = indexOf(e.id);
		if (index == -1) return;
		int lastIndex = count - 1;
		if (index != lastIndex)
		{
			std::swap(components[index], components[lastIndex]);
			std::swap(generations[index], generations[lastIndex]);
			std::swap(indexToEntity[index], indexToEntity[lastIndex]);
			entityToIndex[indexToEntity[index]] = index;
		}
		components.pop_back();
		generations.pop_back();
		indexToEntity.pop_back();
		entityToIndex[e.id] = -1;
		count--;
	}
	inline bool hasComponent(Entity e) const
	{
		int index = indexOf(e.id);
		return index != -1 && generations[index] == e.generation;
	}
	/// <summary>
	/// 持っていない時は既定値のダミーを返す(呼ぶ前に hasComponent か each で確かめる)
	/// </summary>
	inline T& getComponent(Entity e)
	{
		if (!hasComponent(e))
		{
			static T fallback{};
			fallback = T{};
			return fallback;
		}
		return components[entityToIndex[e.id]];
	}
	inline unsigned int getEntityId(int index) const
	{
		return static_cast<unsigned int>(indexToEntity[index]);
	}
	inline unsigned int getGeneration(int index) const
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
		entityToIndex.assign(MAX_ENTITIES, -1);
		indexToEntity.clear();
		generations.clear();
		count = 0;
	}
};
