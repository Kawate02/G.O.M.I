#include "UIPrefab.h"
#include "UIButton.h"
#include "UIImage.h"
#include "UIText.h"
#include "EventBus.h"

#include "UIManager.h"

UIHandle UIPrefab_Title(UIManager& manager, const UISpawnDef& def)
{
	UIImage pernt("", def.pos, { 1, 1 }, def.layer, 0, false);
	UIText text("", 42, Anchor::CENTER, { def.pos.x + 400, def.pos.y + 300 }, def.layer, 1, true);

	UIButton child1("", "ui_hover", "ui_hover", { def.pos.x + 400 - 64, def.pos.y + 400 - 28 }, { 2, 2 }, def.layer, 1, []() { EventBus::getInstance().emit<int>("WorldChange", 1); }, false);
	UIText childText("START", 32, Anchor::CENTER, { def.pos.x + 400, def.pos.y + 400 }, def.layer, 2, false);
	UIButton child2("", "ui_hover", "ui_hover", { def.pos.x + 400 - 64, def.pos.y + 470 - 28 }, { 2, 2 }, def.layer, 1, []() { EventBus::getInstance().emit<int>("quit", 0); }, false);
	UIText childText2("EXIT", 32, Anchor::CENTER, { def.pos.x + 400, def.pos.y + 470 }, def.layer, 2, false);

	auto handle = manager.add(std::make_unique<UIImage>(std::move(pernt)));
	manager.addChild(handle, std::make_unique<UIText>(text));
	manager.addChild(handle, std::make_unique<UIButton>(child1));
	manager.addChild(handle, std::make_unique<UIButton>(child2));
	manager.addChild(handle, std::make_unique<UIText>(childText));
	manager.addChild(handle, std::make_unique<UIText>(childText2));

	return handle;
}