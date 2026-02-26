#include "BackGroundSystem.h"
#include "World.h"
#include "SettingManager.h"

float BackGroundSystem::posMod(float pos, float size)
{
	float mod = fmod(pos, size);
	if (mod < 0) mod += size;
	return mod;
}

void BackGroundSystem::initialize(World& world)
{
}

void BackGroundSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& bg = world.getComponentManager().getComponentArray<BackGroundComponent>();

	world.getComponentManager().each<BackGroundComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& bgc = bg.getComponent(e);
		if (!bgc.enable) return;

		Vec2 rectSize = assetTable.at(bgc.asset).size;
		const float width = rectSize.x;
		const float height = rectSize.y;
		if (width <= 0 || height <= 0) return;

		const float cameraX = world.getCamera().position.x * bgc.parallax.x;
		const float cameraY = world.getCamera().position.y * bgc.parallax.y;

		const float baseX = bgc.origin.x + cameraX;
		const float baseY = bgc.origin.y + cameraY;

		const float modX = posMod(baseX, width);
		const float modY = posMod(baseY, height);

		float startX = -modX;
		float startY = -modY;

		const int screenX = SettingManager::getInstance().getValue<int>("windowWidth");
		const int screenY = SettingManager::getInstance().getValue<int>("windowHeight");

		const int tilesX = (int)ceil((screenX + (bgc.margin * 2 + 1) * width) / width);
		const int tilesY = (int)ceil((screenY + (bgc.margin * 2 + 1) * height) / height);

		for (int y = 0; y < tilesY; y++)
		{
			for (int x = 0; x < tilesX; x++)
			{
				SpriteRenderCommand cmd{
					.assetID = bgc.asset,
					.position = {
						startX + x * width,
						startY + y * height
					},
					.key{
						.layer = bgc.layer,
						.order = bgc.order,
						.useCamera = false
						}};

				world.getRenderQueue().addCommand(cmd);
			}
		}
		});
}
