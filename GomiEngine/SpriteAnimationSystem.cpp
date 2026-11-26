#include "SpriteAnimationSystem.h"
#include "AnimationClip.h"
#include "ComponentType.h"
#include "World.h"

#include <cmath>

void SpriteAnimationSystem::initialize(World& world)
{
}

static const AnimationClip* currentClip(const SpriteAnimationComponent& anim)
{
	if (anim.clip.empty()) return nullptr;
	auto it = anim.clips.find(anim.clip);
	if (it == anim.clips.end())
	{
		AssetDatabase::getInstance().warnOnce("animation:clip:" + anim.clip, "SpriteAnimation: clip \"" + anim.clip + "\" is not in clips");
		return nullptr;
	}
	return AnimationClip::load(it->second);
}

void SpriteAnimationSystem::update(World& world, int frameCount, float deltaTime)
{
	auto& anims = world.getComponentManager().getComponentArray<SpriteAnimationComponent>();
	auto& renderers = world.getComponentManager().getComponentArray<RendererComponent>();

	world.getComponentManager().each<SpriteAnimationComponent, RendererComponent>([&](Entity e) {
		if (!world.alive(e)) return;
		auto& anim = anims.getComponent(e);
		const AnimationClip* clip = currentClip(anim);
		if (!clip) return;
		auto& renderer = renderers.getComponent(e);
		//今の time のコマを書いてから進める(play した次のフレームに最初のコマが出る)
		const AnimationFrame& frame = clip->frames[clip->frameAt(anim.time)];
		renderer.path = frame.image;
		renderer.uv.assign(frame.uv.begin(), frame.uv.end());
		if (anim.playing)
		{
			anim.time += deltaTime * anim.speed;
			if (anim.time < 0) anim.time = 0;
			if (!clip->loop && anim.time > clip->duration()) anim.time = clip->duration();
			if (clip->loop && anim.time >= clip->duration()) anim.time = std::fmod(anim.time, clip->duration()); //増え続けて精度が落ちないように
		}
		});
}

void playAnimation(World& world, Entity entity, const std::string& name, bool restart)
{
	auto& anims = world.getComponentManager().getComponentArray<SpriteAnimationComponent>();
	if (!world.alive(entity) || !anims.hasComponent(entity))
	{
		AssetDatabase::getInstance().warnOnce("playAnimation:noanim:" + name, "playAnimation: the entity has no SpriteAnimation (\"" + name + "\")");
		return;
	}
	auto& anim = anims.getComponent(entity);
	if (anim.clips.find(name) == anim.clips.end())
	{
		AssetDatabase::getInstance().warnOnce("playAnimation:unknown:" + name, "playAnimation: unknown clip name \"" + name + "\"");
		return;
	}
	if (anim.clip == name && !restart) return;
	anim.clip = name;
	anim.time = 0;
	anim.playing = true;
}

bool isAnimationFinished(World& world, Entity entity)
{
	auto& anims = world.getComponentManager().getComponentArray<SpriteAnimationComponent>();
	if (!world.alive(entity) || !anims.hasComponent(entity)) return false;
	auto& anim = anims.getComponent(entity);
	const AnimationClip* clip = currentClip(anim);
	return clip && clip->finishedAt(anim.time);
}
