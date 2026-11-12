// GomiGame.dll にだけ入る入口。エンジン(GameModule)が GetProcAddress で呼ぶ
#include "GameCode.h"

std::vector<GameRegisterFunc>& gomiGameRegistrations()
{
	static std::vector<GameRegisterFunc> list;
	return list;
}

extern "C" __declspec(dllexport) void gomiRegisterGame(ComponentRegistry& components, SystemRegistry& systems)
{
	for (auto f : gomiGameRegistrations()) f(components, systems);
}
