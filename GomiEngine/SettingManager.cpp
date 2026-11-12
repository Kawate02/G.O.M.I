#include "SettingManager.h"
#include "GameModule.h"

std::filesystem::path SettingManager::path() const
{
	return GameModule::exeDir() / filename;
}

void SettingManager::loadSettings()
{
	std::ifstream file(path());
	if (file.is_open())
	{
		try
		{
			nlohmann::json j;
			file >> j;
			for (auto& [key, value] : j.items())
			{
				settings[key] = value.is_string() ? value.get<std::string>() : value.dump();
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "settings.json is broken, using defaults: " << e.what() << std::endl;
		}
	}
}

void SettingManager::saveSettings() const
{
	std::ofstream file(path());
	if (file.is_open())
	{
		nlohmann::json j;
		for (const auto& pair : settings)
		{
			j[pair.first] = pair.second;
		}
		file << j.dump(4);
		file.close();
	}
}

SettingManager& SettingManager::getInstance()
{
	static SettingManager instance;
	return instance;
}
