#include "UIButton.h"

void UIButton::onUpdate(RenderQueue& renderQueue)
{
	std::string res = isPressed ? pressedRes : isHover ? hoverRes : idleRes;
	SpriteRenderCommand cmd = { res, pos, size, 0, { layer, order, false } };
	renderQueue.addCommand(cmd);
}

bool UIButton::hitTest(const InputState& inputState)
{
	//当たり範囲はホバー画像の大きさ x 拡大率(画像は生成後に読めるようになることもあるので毎回求める)
	auto assetSize = AssetDatabase::getInstance().imageSize(hoverRes);
	rectSize = { assetSize.x * size.x, assetSize.y * size.y };
	if (inputState.mouse.position.x > pos.x &&
		inputState.mouse.position.x < pos.x + rectSize.x &&
		inputState.mouse.position.y > pos.y &&
		inputState.mouse.position.y < pos.y + rectSize.y)
	{
		if (!isHover)
		{
			EventBus::getInstance().emit<AudioCommand>("audioPlay", { hoverSound, 100 });
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
			EventBus::getInstance().emit<AudioCommand>("audioPlay", { clickSound, 100 });
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
