#include "UIPrefab.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "EventBus.h"

#include "UIManager.h"

UIHandle UIPrefab_Pause(UIManager& manager, const UISpawnDef& def)
{
	UIImage parent("screen", def.pos, { 1, 1 }, def.layer, 0, true);
	UIText text("PAUSE", 32, Anchor::CENTER, { def.pos.x + 400, def.pos.y + 200 }, def.layer, 1, true);

	UIButton child1("", "ui_hover", "ui_hover", { def.pos.x + 400 - 64, def.pos.y + 300 - 28 }, { 2, 2 }, def.layer, 1, []() { EventBus::getInstance().emit<int>("PauseClose", 0); }, true);
	UIText childText("RESUME", 32, Anchor::CENTER, { def.pos.x + 400, def.pos.y + 300 }, def.layer, 2, true);
	UIButton child2("", "ui_hover", "ui_hover", { def.pos.x + 400 - 64, def.pos.y + 400 - 28 }, { 2, 2 }, def.layer, 1, []() { EventBus::getInstance().emit<int>("WorldChange", 0); }, true);
	UIText childText2("EXIT", 32, Anchor::CENTER, { def.pos.x + 400, def.pos.y + 400 }, def.layer, 2, true);

	auto handle = manager.add(std::make_unique<UIImage>(std::move(parent)));
	manager.addChild(handle, std::make_unique<UIText>(text), UIType::RESULT);
	manager.addChild(handle, std::make_unique<UIButton>(child1));
	manager.addChild(handle, std::make_unique<UIButton>(child2));
	manager.addChild(handle, std::make_unique<UIText>(childText));
	manager.addChild(handle, std::make_unique<UIText>(childText2));

	return handle;
}