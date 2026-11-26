#include "OffScreenSystem.h"
#include "World.h"
#include "SettingManager.h"

Vec2 OffScreenSystem::imageSize(World& world, Entity e)
{
	auto& rend = world.getComponentManager().getComponentArray<RendererComponent>();
	if (!rend.hasComponent(e)) return {};
	return rendererImageSize(rend.getComponent(e));
}

void OffScreenSystem::despawn(World& world, Entity e)
{
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>().getComponent(e);
	Vec2 size = imageSize(world, e);
	if (tf.position.x + size.x < cameraRect.left) world.destroyEntity(e);
	if (tf.position.x > cameraRect.right) world.destroyEntity(e);
	if (tf.position.y + size.y < cameraRect.top) world.destroyEntity(e);
	if (tf.position.y > cameraRect.bottom) world.destroyEntity(e);
}

void OffScreenSystem::clamp(World& world, Entity e)
{
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>().getComponent(e);
	auto& moves = world.getComponentManager().getComponentArray<MoveComponent>();
	auto stop = [&](int axis) { if (moves.hasComponent(e)) moves.getComponent(e).physVel[axis] = 0; };

	Vec2 size = imageSize(world, e);
	if (tf.position.x + size.x > cameraRect.right)
	{
		tf.position.x = cameraRect.right - size.x;
		stop(0);
	}
	if (tf.position.x < cameraRect.left)
	{
		tf.position.x = static_cast<float>(cameraRect.left);
		stop(0);
	}
	if (tf.position.y + size.y > cameraRect.bottom)
	{
		tf.position.y = cameraRect.bottom - size.y;
		stop(1);
	}
	if (tf.position.y < cameraRect.top)
	{
		tf.position.y = static_cast<float>(cameraRect.top);
		stop(1);
	}
}

void OffScreenSystem::wrap(World& world, Entity e)
{
	
}

void OffScreenSystem::initialize(World& world)
{
}

void OffScreenSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& offscreen = world.getComponentManager().getComponentArray<OffScreenComponent>();
	auto& cam = world.getCamera();
	int cameraW = SettingManager::getInstance().getValue<int>("windowWidth");
	int cameraH = SettingManager::getInstance().getValue<int>("windowHeight");
	cameraRect = { static_cast<int>(cam.position.x), static_cast<int>(cam.position.y), static_cast<int>(cam.position.x) + cameraW,  static_cast<int>(cam.position.y) + cameraH };

	world.getComponentManager().each<OffScreenComponent, TransformComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		switch (offscreen.getComponent(e).mode)
		{
		case OffScreenMode::CLAMP: clamp(world, e); break;
		case OffScreenMode::DESPAWN: despawn(world, e); break;
		}
		});
}
