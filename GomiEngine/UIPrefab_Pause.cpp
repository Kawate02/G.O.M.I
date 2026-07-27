#include "UIPrefab_Pause.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "EventBus.h"

const UIPrefabDefinition UIPrefab_Pause = {
	{
		{ Widget(UIImage("screen", { 0, 0 }, { 1, 1 }, 1, 0, true)) },
		{ Widget(UIText("PAUSE", 32, Anchor::CENTER, { 400, 200 }, 1, 1, true)), UIType::RESULT },
		{ Widget(UIButton("", "ui_hover", "ui_hover", { 400 - 64, 300 - 28 }, { 2, 2 }, 1, 1, []() { EventBus::getInstance().emit<int>("PauseClose", 0); }, true)) },
		{ Widget(UIText("RESUME", 32, Anchor::CENTER, { 400, 300 }, 1, 2, true)) },
		{ Widget(UIButton("", "ui_hover", "ui_hover", { 400 - 64, 400 - 28 }, { 2, 2 }, 1, 1, []() { EventBus::getInstance().emit<std::string>("WorldChange", "Title"); }, true)) },
		{ Widget(UIText("EXIT", 32, Anchor::CENTER, { 400, 400 }, 1, 2, true)) },
	}
};
