#include "UIPrefab.h"
#include "UIImage.h"

#include "UIManager.h"

UIHandle UIPrefab_Image(UIManager& manager, const UISpawnDef& def)
{
	UIImage pernt("ui_frame", def.pos, { 4, 4 }, def.layer, 0, false);

	return manager.add(std::make_unique<UIImage>(std::move(pernt)));
}