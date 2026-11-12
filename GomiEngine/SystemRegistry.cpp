#include "SystemRegistry.h"
#include "SystemInclude.h"

#include <stdexcept>

std::unique_ptr<ISystem> SystemRegistry::create(const std::string& name) const
{
	for (auto& [n, factory] : factories)
	{
		if (n == name) return factory();
	}
	throw std::runtime_error("unknown system \"" + name + "\"");
}

bool SystemRegistry::contains(const std::string& name) const
{
	for (auto& [n, factory] : factories)
	{
		if (n == name) return true;
	}
	return false;
}

void SystemRegistry::truncate(size_t count)
{
	if (count < factories.size()) factories.erase(factories.begin() + count, factories.end());
}

std::vector<std::string> SystemRegistry::names() const
{
	std::vector<std::string> result;
	for (auto& [n, factory] : factories) result.push_back(n);
	return result;
}

// 新しい System を作ったら、ここに名前を足すとシーンファイルで使えるようになる
void registerBuiltinSystems()
{
	static bool done = false;
	if (done) return;
	done = true;

	auto& r = SystemRegistry::getInstance();
	r.add<PlayerControlSystem>("PlayerControlSystem");
	r.add<ForwardMoveSystem>("ForwardMoveSystem");
	r.add<PhysicsSystem>("PhysicsSystem");
	r.add<MoveResolveSystem>("MoveResolveSystem");
	r.add<MoveSystem>("MoveSystem");
	r.add<CameraSystem>("CameraSystem");
	r.add<OffScreenSystem>("OffScreenSystem");
	r.add<RendererSystem>("RendererSystem");
	r.add<CollisionSystem>("CollisionSystem");
	r.add<BackGroundSystem>("BackGroundSystem");
	r.add<MeshRendererSystem>("MeshRendererSystem");
}

SystemRegistry& SystemRegistry::getInstance()
{
	static SystemRegistry instance;
	return instance;
}
