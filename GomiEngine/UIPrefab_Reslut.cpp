#include "UIPrefab_Reslut.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "EventBus.h"

const UIPrefabDefinition UIPrefab_Result = {
	{
		{ Widget(UIImage("screen", { 0, 0 }, { 1, 1 }, 0, 0, true)) },
		{ Widget(UIText("", 32, Anchor::CENTER, { 400, 200 }, 0, 1, true)), UIType::RESULT },
		{ Widget(UIButton("", "ui_hover", "ui_hover", { 400 - 64, 300 - 28 }, { 2, 2 }, 0, 1, []() { EventBus::getInstance().emit<std::string>("WorldChange", "Field"); }, true)) },
		{ Widget(UIText("RESTART", 32, Anchor::CENTER, { 400, 300 }, 0, 2, true)) },
		{ Widget(UIButton("", "ui_hover", "ui_hover", { 400 - 64, 400 - 28 }, { 2, 2 }, 0, 1, []() { EventBus::getInstance().emit<std::string>("WorldChange", "Title"); }, true)) },
		{ Widget(UIText("EXIT", 32, Anchor::CENTER, { 400, 400 }, 0, 2, true)) },
	}
};
