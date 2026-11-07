#include "MeshRendererSystem.h"
#include "World.h"
#include "MathUtil.h"

void MeshRendererSystem::initialize(World& world)
{
}

void MeshRendererSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& cm = world.getComponentManager();
	auto& tf = cm.getComponentArray<TransformComponent>();
	auto& queue = world.getRenderQueue();

	bool cameraFound = false;
	auto& cameras = cm.getComponentArray<Camera3DComponent>();
	cm.each<Camera3DComponent, TransformComponent>([&](Entity e) {
		if (cameraFound || !world.alive(e)) return;
		auto& cam = cameras.getComponent(e);
		if (!cam.active) return;
		auto& t = tf.getComponent(e);
		glm::mat4 cameraToWorld = glm::translate(glm::mat4(1.0f), MathUtil::toGlm(t.position)) * MathUtil::rotationMatrix(t.rotation);
		queue.setCamera3D({ glm::inverse(cameraToWorld), cam.fov, cam.nearClip, cam.farClip, cam.clearColor });
		cameraFound = true;
	});

	bool lightFound = false;
	auto& lights = cm.getComponentArray<LightComponent>();
	cm.each<LightComponent>([&](Entity e) {
		if (lightFound || !world.alive(e)) return;
		auto& l = lights.getComponent(e);
		queue.setLight({ l.direction, l.color * l.intensity, 1.0f, l.ambient });
		lightFound = true;
	});

	auto& renderers = cm.getComponentArray<MeshRendererComponent>();
	cm.each<MeshRendererComponent, TransformComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& r = renderers.getComponent(e);
		if (!r.enable) return;
		auto& t = tf.getComponent(e);
		queue.addMesh({ r.mesh, r.texture, MathUtil::modelMatrix(t.position, t.rotation, t.scale), r.color });
	});
}
