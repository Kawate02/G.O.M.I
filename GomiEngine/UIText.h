#pragma once
#include "UIBase.h"
#include <string>
#include "TextAnchor.h"

class UIText : public UIBase
{
protected:
	std::string text;
	int fontSize = 16;
	Anchor anchor = Anchor::CENTER;
	void changeText(const std::string& text, int fontSize, Anchor anchor);
	void onUpdate(RenderQueue& renderQueue) override;
public:
	UIText(std::string text, int fonsSize, Anchor anchor, const Vec2& pos, int layer, int order, bool blockInput) 
		: text(text), fontSize(fonsSize), anchor(anchor), UIBase(pos, { 0, 0 }, layer, order, blockInput) {}
	void setText(const std::string& text) { changeText(text, fontSize, anchor); }
};