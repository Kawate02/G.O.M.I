// GameModuleTest が読む見本のゲームのコード(GomiGame.dll と同じ作り)
#include "GameCode.h"

struct SampleHpComponent
{
	int hp = 3;
};

class SampleDamageSystem : public ISystem
{
public:
	void initialize(World&) override {}
	void update(World& world, int, float) override
	{
		world.getComponentManager().each<SampleHpComponent>([&](Entity e) {
			auto& hp = world.getComponentManager().getComponentArray<SampleHpComponent>().getComponent(e);
			if (hp.hp > 0) hp.hp--;
		});
	}
};

GOMI_REGISTER
{
	components.add<SampleHpComponent>("SampleHp").field("hp", &SampleHpComponent::hp);
	systems.add<SampleDamageSystem>("SampleDamageSystem");
}
