#include "UIText.h"

void UIText::changeText(const std::string& text, int fontSize, Anchor anchor)
{
	this->text = text;
	this->fontSize = fontSize;
	this->anchor = anchor;
}

void UIText::onUpdate(RenderQueue& renderQueue)
{
	TextRenderCommand cmd = { text, pos, fontSize, anchor, { layer, order, false } };
	renderQueue.addCommand(cmd);
}
