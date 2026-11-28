#include "RendererSystem.h"
#include "World.h"
#include "MathUtil.h"

Vec2 rendererImageSize(const RendererComponent& renderer)
{
	Vec2 size = AssetDatabase::getInstance().imageSize(renderer.path);
	if (renderer.uv.size() == 4)
	{
		size.x *= std::abs(renderer.uv[2] - renderer.uv[0]);
		size.y *= std::abs(renderer.uv[3] - renderer.uv[1]);
	}
	return size;
}

void RendererSystem::initialize(World& world)
{
}

void RendererSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& renderer = world.getComponentManager().getComponentArray<RendererComponent>();
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>();

	world.getComponentManager().each<RendererComponent, TransformComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tfc = tf.getComponent(e);
		auto& rendc = renderer.getComponent(e);
		if (!rendc.enable) return;
		SpriteRenderCommand std = { .assetID = renderer.getComponent(e).path, .position = { tfc.position.x, tfc.position.y }, .scale = { tfc.scale.x, tfc.scale.y }, .angle = MathUtil::toRadians(tfc.rotation.z), .key = {.layer = rendc.layer, .order = rendc.order, .useCamera = rendc.useCamera } };
		if (rendc.uv.size() == 4) std::copy(rendc.uv.begin(), rendc.uv.end(), std.uv.begin());
		std.flipX = rendc.flipX;
		if (rendc.color.size() == 4) std::copy(rendc.color.begin(), rendc.color.end(), std.color.begin());
		if (rendc.flashColor.size() == 3) std::copy(rendc.flashColor.begin(), rendc.flashColor.end(), std.flash.begin());
		std.flash[3] = rendc.flash;
		world.getRenderQueue().addCommand({ std });
		});
}
