#include "UIBase.h"

void UIBase::update(const InputState& inputState, RenderQueue& renderQueue)
{
	if (!visible) return;
	onUpdate(renderQueue);
}
