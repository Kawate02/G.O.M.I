#include "UIButton.h"

void UIButton::onUpdate(RenderQueue& renderQueue)
{
	std::string res = isPressed ? pressedRes : isHover ? hoverRes : idleRes;
	SpriteRenderCommand cmd = { res, pos, size, 0, { layer, order, false } };
	renderQueue.addCommand(cmd);
}

bool UIButton::hitTest(const InputState& inputState)
{
	if (inputState.mouse.position.x > pos.x &&
		inputState.mouse.position.x < pos.x + rectSize.x &&
		inputState.mouse.position.y > pos.y &&
		inputState.mouse.position.y < pos.y + rectSize.y)
	{
		if (!isHover)
		{
			EventBus::getInstance().emit<AudioCommand>("audioPlay", { "cursor_move", 100 });
		}
		isHover = true;
		if (inputState.isPressedKey(InputKey::MOUSE_LEFT))
		{
			isPressed = true;
		}
		else
		{
			isPressed = false;
		}
		if (inputState.isUpKey(InputKey::MOUSE_LEFT))
		{
			isClicked = true;
		}
		else
		{
			isClicked = false;
		}
		if (isClicked && onClick)
		{
			EventBus::getInstance().emit<AudioCommand>("audioPlay", { "cursor_click", 100 });
			onClick();
			return true;
		}
	}
	else
	{
		isHover = false;
	}
	return false;
}
