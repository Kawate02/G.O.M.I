#include "SettingManager.h"

void SettingManager::loadSettings()
{
	std::ifstream file(filename);
	if (file.is_open())
	{
		nlohmann::json j;
		file >> j;
		settings = j;
		file.close();
	}
}

void SettingManager::saveSettings() const
{
	std::ofstream file(filename);
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
