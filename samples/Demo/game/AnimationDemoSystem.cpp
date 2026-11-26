// コマ送りアニメーションの見本: A・D で歩く(クリップ walk、止まると idle)、スペースでジャンプ(jump は1回流して終わる)。
// どのクリップを流すかはここで決め、SpriteAnimation の項目は直接書かず playAnimation / isAnimationFinished だけを使う。
// 動かすのはシーンの systems の PlayerControlSystem など。シーンでは SpriteAnimationSystem を RendererSystem より前に置く
#include "GameCode.h"

/// <summary>AnimationDemoSystem が操作する印(ジャンプ中かどうかだけ持つ)</summary>
struct AnimationDemoComponent
{
	bool jumping = false;
};

class AnimationDemoSystem : public ISystem
{
public:
	void initialize(World&) override {}

	void update(World& world, int, float) override
	{
		auto& cm = world.getComponentManager();
		auto& demos = cm.getComponentArray<AnimationDemoComponent>();
		auto& renderers = cm.getComponentArray<RendererComponent>();
		auto& input = world.getInputState();
		cm.each<AnimationDemoComponent>([&](Entity e) {
			if (!world.alive(e)) return;
			auto& demo = demos.getComponent(e);
			bool left = input.isPressedKey(InputKey::KEY_A);
			bool right = input.isPressedKey(InputKey::KEY_D);
			if (renderers.hasComponent(e) && left != right) renderers.getComponent(e).flipX = left;

			if (input.isDownKey(InputKey::KEY_SPACE) && !demo.jumping)
			{
				demo.jumping = true;
				playAnimation(world, e, "jump", true);
			}
			if (demo.jumping)
			{
				if (!isAnimationFinished(world, e)) return;
				demo.jumping = false;
			}
			playAnimation(world, e, (left != right) ? "walk" : "idle");
			});
	}
};

GOMI_REGISTER
{
	components.add<AnimationDemoComponent>("AnimationDemo").runtime("jumping", &AnimationDemoComponent::jumping);
	systems.add<AnimationDemoSystem>("AnimationDemoSystem");
}
