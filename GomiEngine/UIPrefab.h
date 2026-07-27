#pragma once
#include "UIManager.h"
#include "Vec2.h"

#include <vector>
#include <functional>
#include <memory>

using UIFactory = std::function<std::unique_ptr<UIBase>()>;

template <typename T>
UIFactory Widget(T widget)
{
	return [widget]() { return std::make_unique<T>(widget); };
}

struct UIChildDefinition
{
	UIFactory factory;
	UIType type = UIType::NONE;
};

struct UIPrefabDefinition
{
	std::vector<UIChildDefinition> children;
};

struct UISpawnOverride
{
	const UIPrefabDefinition* prefab;
	Vec2 pos;
	int layer = 0;
};
