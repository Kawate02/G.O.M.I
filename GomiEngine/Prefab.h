#pragma once
#include "ComponentManager.h"
#include "Vec3.h"

#include <vector>
#include <functional>
#include <optional>

using ComponentFactory = std::function<void(ComponentManager&, Entity)>;

template <typename T>
ComponentFactory Component(T value)
{
	return [value](ComponentManager& cm, Entity e) { cm.addComponent<T>(e, value); };
}

struct EntityDefinition
{
	std::vector<ComponentFactory> components;
};

struct PrefabDefinition
{
	std::vector<EntityDefinition> entities;
};

struct SpawnOverride
{
	const PrefabDefinition* prefab;
	std::optional<Vec3> pos;
	Vec3 scale = { 1.0f, 1.0f, 1.0f };
	float rot = 0.0f;
};
