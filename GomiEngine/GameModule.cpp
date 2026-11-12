#include "GameModule.h"
#include "ComponentRegistry.h"
#include "SystemRegistry.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using RegisterGameFunc = void(*)(ComponentRegistry&, SystemRegistry&);

GameModule::~GameModule()
{
	unload();
}

bool GameModule::load(const std::filesystem::path& dll, std::string& error)
{
	unload();
	auto& components = ComponentRegistry::getInstance();
	auto& systems = SystemRegistry::getInstance();
	HMODULE module = LoadLibraryW(dll.wstring().c_str());
	if (!module)
	{
		error = "cannot load " + dll.string() + " (error " + std::to_string(GetLastError()) + ")";
		return false;
	}
	auto func = reinterpret_cast<RegisterGameFunc>(GetProcAddress(module, "gomiRegisterGame"));
	if (!func)
	{
		FreeLibrary(module);
		error = dll.string() + " has no gomiRegisterGame";
		return false;
	}
	componentBase = components.size();
	systemBase = systems.size();
	try
	{
		func(components, systems);
	}
	catch (const std::exception& ex)
	{
		components.truncate(componentBase);
		systems.truncate(systemBase);
		FreeLibrary(module);
		error = ex.what();
		return false;
	}
	handle = module;
	loadedPath = dll;
	return true;
}

void GameModule::unload()
{
	if (!handle) return;
	ComponentRegistry::getInstance().truncate(componentBase);
	SystemRegistry::getInstance().truncate(systemBase);
	FreeLibrary(static_cast<HMODULE>(handle));
	handle = nullptr;
	loadedPath.clear();
}

std::filesystem::path GameModule::exeDir()
{
	wchar_t buffer[MAX_PATH];
	DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	return std::filesystem::path(std::wstring(buffer, n)).parent_path();
}
