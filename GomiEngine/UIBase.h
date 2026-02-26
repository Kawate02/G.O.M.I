#pragma once
#include "Vec2.h"
#include "InputState.h"
#include "Utilities.h"
#include "RenderQueue.h"
#include "AssetTable.h"
#include "EventBus.h"
#include "AudioCommand.h"
#include "UIHandle.h"
#include <vector>

class UIBase
{
protected:
	Vec2 pos;
	Vec2 size;
	Vec2 rectSize = { 64, 64 };
	bool visible = true;
	bool enabled = true;
	std::vector<UIHandle> childHandles;

	bool blockInput = false;

	int layer = 1000;
	int order = 0;

	virtual void onUpdate(RenderQueue& renderQueue) {}
public:
	virtual bool hitTest(const InputState& inputState) { return false; }
	UIBase(const Vec2& pos, const Vec2& size, int layer, int order, bool blockInput) : pos(pos), size(size), layer(layer + 1000), order(order), blockInput(blockInput) {}
	inline void addChild(UIHandle handle) { childHandles.push_back(handle); }
	inline const std::vector<UIHandle>& getChildren() const { return childHandles; }
	void update(const InputState& inputState, RenderQueue& renderQueue);
	inline void setVisible(bool visible) { this->visible = visible; }
	inline void setEnabled(bool enabled) { this->enabled = enabled; }
	inline int getLayer() const { return layer; }
	inline int getOrder() const { return order; }
	inline bool block() const { return blockInput; }
	inline void clear() { childHandles.clear(); }
	virtual void setImage(std::string res) {};
	virtual void setText(const std::string& text) {};
};