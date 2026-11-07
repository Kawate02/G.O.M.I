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
	auto& camera3D = queue.getCamera3D();
	if (camera3D) drawer.clearScreen(camera3D->clearColor.x, camera3D->clearColor.y, camera3D->clearColor.z);
	else drawer.clearScreen(0, 0, 0);

	//3D(カメラがある時だけ)
	if (camera3D && !queue.getMeshes().empty())
	{
		auto& light = queue.getLight();
		Scene3DParams params;
		params.view = camera3D->view;
		params.fov = camera3D->fov;
		params.nearClip = camera3D->nearClip;
		params.farClip = camera3D->farClip;
		params.lightDirection = MathUtil::toGlm(light.direction);
		params.lightColor = MathUtil::toGlm(light.color) * light.intensity;
		params.ambient = light.ambient;
		drawer.beginScene3D(params);
		auto& resources = ResourceManager::getInstance();
		for (auto& m : queue.getMeshes())
		{
			int mesh = resources.loadMesh(drawer, m.mesh);
			if (mesh == -1) continue;
			int texture = m.texture.empty() ? -1 : resources.loadImage(drawer, m.texture).handle;
			drawer.drawMesh(mesh, texture, m.model, MathUtil::toGlm(m.color));
		}
		drawer.endScene3D();
	}

	//2D(スプライト・文字・UI)は 3D の上に重ねる
	auto items = queue.getCommands();
	queue.clear();

	std::stable_sort(items.begin(), items.end(), [](const RenderCommand& a, const RenderCommand& b)
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
			auto res = ResourceManager::getInstance().loadImage(drawer, sprite->assetID);
			if (res.handle == -1) continue;
			drawer.drawImage(res.handle, sprite->position.x, sprite->position.y, res.width * sprite->scale.x, res.height * sprite->scale.y, sprite->angle);
		}
		if (TextRenderCommand* text = std::get_if<TextRenderCommand>(&item))
		{
			if (text->key.useCamera)
			{
				text->position -= camera.position;
			}
			drawer.drawText(text->text, text->fontSize, text->anchor, text->position.x, text->position.y);
		}
	}
}
