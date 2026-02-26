#pragma once
#include "UIBase.h"
#include <functional>

class UIButton : public UIBase
{
protected:
	std::string idleRes = "";
	std::string hoverRes = "";
	std::string pressedRes = "";
	bool isHover = false;
	bool isPressed = false;
	bool isClicked = false;
	std::function<void()> onClick;

	void onUpdate(RenderQueue& renderQueue) override;
public:
	UIButton(std::string idleRes, std::string hoverRes, std::string pressedRes, const Vec2& pos, const Vec2& size, int layer, int order, std::function<void()> onClick, bool blockInput) 
		: idleRes(idleRes), hoverRes(hoverRes), pressedRes(pressedRes), onClick(onClick), UIBase(pos, size, layer, order, blockInput)
	{
		auto assetSize = assetTable.at(hoverRes).size;
		rectSize = { assetSize.x * size.x, assetSize.y * size.y };
	}
	bool hitTest(const InputState& inputState) override;
};