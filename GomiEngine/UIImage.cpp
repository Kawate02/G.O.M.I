#include "UIImage.h"

void UIImage::onUpdate(RenderQueue& renderQueue)
{
	SpriteRenderCommand cmd = { imageRes, pos, size, 0, { layer, order, false } };
	renderQueue.addCommand(cmd);
}
