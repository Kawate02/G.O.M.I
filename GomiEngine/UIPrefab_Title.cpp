#include "UIPrefab_Title.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "EventBus.h"

const UIPrefabDefinition UIPrefab_Title = {
	{
		{ Widget(UIText("", 42, Anchor::CENTER, { 400, 300 }, 0, 1, true)) },
		{ Widget(UIButton("", "ui_hover", "ui_hover", { 400 - 64, 400 - 28 }, { 2, 2 }, 0, 1, []() { EventBus::getInstance().emit<std::string>("WorldChange", "Field"); }, false)) },
		{ Widget(UIText("START", 32, Anchor::CENTER, { 400, 400 }, 0, 2, false)) },
		{ Widget(UIButton("", "ui_hover", "ui_hover", { 400 - 64, 470 - 28 }, { 2, 2 }, 0, 1, []() { EventBus::getInstance().emit<int>("quit", 0); }, false)) },
		{ Widget(UIText("EXIT", 32, Anchor::CENTER, { 400, 470 }, 0, 2, false)) },
	}
};
