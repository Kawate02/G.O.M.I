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
	std::string hoverSound = "sounds/cursor_move.wav";
	std::string clickSound = "sounds/cursor_click.wav";

	void onUpdate(RenderQueue& renderQueue) override;
public:
	UIButton(std::string idleRes, std::string hoverRes, std::string pressedRes, const Vec2& pos, const Vec2& size, int layer, int order, std::function<void()> onClick, bool blockInput) 
		: idleRes(idleRes), hoverRes(hoverRes), pressedRes(pressedRes), onClick(onClick), UIBase(pos, size, layer, order, blockInput)
	{
	}
	bool hitTest(const InputState& inputState) override;
	inline void setSounds(const std::string& hover, const std::string& click) { hoverSound = hover; clickSound = click; }
};