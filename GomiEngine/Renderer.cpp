#include "Renderer.h"
#include <algorithm>

const RenderKey& getKey(const RenderCommand& command)
{
	return std::visit([](const auto& c) -> const RenderKey& { return c.key; }, command);
}

void Renderer::init()
{
	
}

void Renderer::render(IDrawer& drawer, RenderQueue& queue, const Camera& camera)
{
	drawer.clearScreen();
	auto items = queue.getCommands();
	queue.clear();

	std::sort(items.begin(), items.end(), [](const RenderCommand& a, const RenderCommand& b) 
		{ 
			const RenderKey& aKey = getKey(a);
			const RenderKey& bKey = getKey(b);
			if (aKey.layer != bKey.layer)
			{
				return aKey.layer < bKey.layer;
			}
			return aKey.order < bKey.order;
		});

	for (auto& item : items)
	{
		if (SpriteRenderCommand* sprite = std::get_if<SpriteRenderCommand>(&item))
		{
			if (sprite->key.useCamera)
			{
				sprite->position -= camera.position;
			}
			auto res = ResourceManager::getInstance().loadImage(sprite->assetID);
			if (res.handle == -1) continue;
			drawer.drawImage(res.handle, sprite->position.x, sprite->position.y, res.width, res.height, sprite->scale.x, sprite->scale.y, sprite->angle);
		}
		if (TextRenderCommand* text = std::get_if<TextRenderCommand>(&item))
		{
			if (text->key.useCamera)
			{
				text->position -= camera.position;
			}
			drawer.drawText(text->text, text->fontSize,text->anchor, text->position.x, text->position.y);
		}
	}
	drawer.present();
}
