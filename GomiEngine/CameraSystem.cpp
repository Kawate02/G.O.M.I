#include "CameraSystem.h"
#include "World.h"
#include "MathUtil.h"

void CameraSystem::initialize(World& world)
{
}

void CameraSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& cm = world.getComponentManager();
	auto& fEntity = cm.getComponentArray<CameraFollowComponent>();
	auto& tf = cm.getComponentArray<TransformComponent>();
	auto& camera = world.getCamera();

	//3D: 使っているカメラ(Camera3D)の Entity
	Entity camera3D{};
	bool hasCamera3D = false;
	auto& cameras = cm.getComponentArray<Camera3DComponent>();
	cm.each<Camera3DComponent, TransformComponent>([&](Entity e) {
		if (hasCamera3D || !world.alive(e) || !cameras.getComponent(e).active) return;
		camera3D = e;
		hasCamera3D = true;
	});

	cm.each<CameraFollowComponent, TransformComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& tfc = tf.getComponent(e);
		auto& fEntityc = fEntity.getComponent(e);

		//2D: 画面の左上を対象 + offset に合わせる
		if (fEntityc.x) camera.position.x = tfc.position.x + fEntityc.offset.x;
		if (fEntityc.y) camera.position.y = tfc.position.y + fEntityc.offset.y;

		//3D: カメラを対象 + offset に置いて対象を見る
		if (hasCamera3D && camera3D != e)
		{
			auto& cam = tf.getComponent(camera3D);
			if (fEntityc.x) cam.position.x = tfc.position.x + fEntityc.offset.x;
			if (fEntityc.y) cam.position.y = tfc.position.y + fEntityc.offset.y;
			if (fEntityc.z) cam.position.z = tfc.position.z + fEntityc.offset.z;
			cam.rotation = MathUtil::lookRotation(cam.position, tfc.position);
		}
		});
}
