#pragma once
#include "ComponentType.h"
#include "Entity.h"
#include "JsonConvert.h"

#include <memory>
#include <vector>
#include <functional>

class World;

class ISystem
{
public:
	virtual void initialize(World& world) = 0;
	virtual void update(World& world, int frameCount, float deltaTime) = 0;
	/// <summary>
	/// エディタで止めている間も動かすか(描画の命令を積むだけの System は true)
	/// </summary>
	virtual bool runsInEditMode() const { return false; }
	/// <summary>
	/// 巻き戻しで覚える System 自身の状態。Entity は保存し直すと ID が変わるので、保存した順(order)の番号で持つ。無ければ null
	/// </summary>
	virtual Json runState(const std::vector<Entity>& order) const { return Json(); }
	/// <summary>
	/// runState で覚えた状態を戻す。order は戻した後の Entity(保存した順と同じ並び)
	/// </summary>
	virtual void restoreRunState(const Json& state, const std::vector<Entity>& order) {}
	virtual ~ISystem() = default;
};

template <typename T>
std::function<std::unique_ptr<ISystem>()> System()
{
	return []() { return std::make_unique<T>(); };
}