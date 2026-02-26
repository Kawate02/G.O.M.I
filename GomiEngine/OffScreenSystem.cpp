#include "OffScreenSystem.h"
#include "World.h"
#include "SettingManager.h"

void OffScreenSystem::despawn(World& world, Entity e)
{
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>().getComponent(e);
	auto& rend = world.getComponentManager().getComponentArray<RendererComponent>().getComponent(e);
	Vec2 size = assetTable.at(rend.path).size;
	if (tf.position.x + size.x < cameraRect.left) world.destroyEntity(e);
	if (tf.position.x > cameraRect.right) world.destroyEntity(e);
	if (tf.position.y + size.y < cameraRect.top) world.destroyEntity(e);
	if (tf.position.y > cameraRect.bottom) world.destroyEntity(e);
}

void OffScreenSystem::clamp(World& world, Entity e)
{
	auto& tf = world.getComponentManager().getComponentArray<TransformComponent>().getComponent(e);
	auto& rend = world.getComponentManager().getComponentArray<RendererComponent>().getComponent(e);
	auto& move = world.getComponentManager().getComponentArray<MoveComponent>().getComponent(e);

	Vec2 size = assetTable.at(rend.path).size;
	if (tf.position.x + size.x > cameraRect.right)
	{
		tf.position.x = cameraRect.right - size.x;
		move.physVel.x = 0;
	}
	if (tf.position.x < cameraRect.left)
	{
		tf.position.x = cameraRect.left;
		move.physVel.x = 0;
	}
	if (tf.position.y + size.y > cameraRect.bottom)
	{
		tf.position.y = cameraRect.bottom - size.y;
		move.physVel.y = 0;
	}
	if (tf.position.y < cameraRect.top)
	{
		tf.position.y = cameraRect.top;
		move.physVel.y = 0;
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
	auto& tag = world.getComponentManager().getComponentArray<EntityTagComponent>();
	auto& cam = world.getCamera();
	int cameraW = SettingManager::getInstance().getValue<int>("windowWidth");
	int cameraH = SettingManager::getInstance().getValue<int>("windowHeight");
	cameraRect = { static_cast<int>(cam.position.x), static_cast<int>(cam.position.y), static_cast<int>(cam.position.x) + cameraW,  static_cast<int>(cam.position.y) + cameraH };

	world.getComponentManager().each<EntityTagComponent, TransformComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tagc = tag.getComponent(e);

		if (tagc.has(EntityTag::PLAYER))
		{
			clamp(world, e);
		}
		else if (tagc.has(EntityTag::BULLET))
		{
			despawn(world, e);
		}
		});
}
