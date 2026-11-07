#pragma once
#include "UIBase.h"

class UIImage : public UIBase
{
protected:
	std::string imageRes = "";
	void onUpdate(RenderQueue& renderQueue) override;
public:
	UIImage(std::string res, const Vec2& pos, const Vec2& size, int layer, int order, bool blockInput)
		: imageRes(res), UIBase(pos, size, layer, order, blockInput)
	{
	}
	void setImage(std::string res) override { imageRes = res; }
};