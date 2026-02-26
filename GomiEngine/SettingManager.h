#pragma once
#include "include/json.hpp"
#include <fstream>

#include "Utilities.h"
#include "EventBus.h"
#include "GameSettings.h"

class SettingManager
{
private:
	SettingManager() = default;
	std::map<std::string, std::string> settings;
	const std::string filename = "settings.json";
public:
	static SettingManager& getInstance()
	{
		static SettingManager instance;
		return instance;
	}

	void loadSettings();
	void saveSettings() const;
	template <typename T>
	T getValue(const std::string& key) const
	{
		auto it = settings.find(key);
		if (it != settings.end())
		{
			if constexpr (std::is_same_v<T, int>)
			{
				return std::stoi(it->second);
			}
			else if constexpr (std::is_same_v<T, float>)
			{
				return std::stof(it->second);
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				return it->second == "true";
			}
			else if constexpr (std::is_same_v<T, std::string>)
			{
				return it->second;
			}
			else
			{
				return T();
			}
		}
	}

	template <typename T>
	void setValue(const std::string& key, T value, const std::string& eventName = "")
	{
		settings[key] = std::to_string(value);
		saveSettings();
		EventBus::getInstance().emit<T>(eventName, value);
	}
};