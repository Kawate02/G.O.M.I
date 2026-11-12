#pragma once
#include "include/json.hpp"
#include <filesystem>
#include <fstream>

#include "Utilities.h"
#include "EventBus.h"
#include "GameSettings.h"

class SettingManager
{
private:
	SettingManager() = default;
	std::map<std::string, std::string> settings;
	const std::string filename = "settings.json"; //遊ぶ人の設定なので exe の隣に置く(プロジェクトには置かない)
	std::filesystem::path path() const;
	const std::map<std::string, std::string> defaults = {
		{ "windowWidth", "800" },
		{ "windowHeight", "600" },
		{ "fullscreen", "false" },
	};
public:
	// DLL をまたいで1つにするため、実体は .cpp に置く(ヘッダの inline だとモジュールごとに別物になる)
	static SettingManager& getInstance();

	void loadSettings();
	void saveSettings() const;
	/// <summary>
	/// 設定値を読む。settings.json に無ければ既定値、それも無ければ T() を返す
	/// </summary>
	template <typename T>
	T getValue(const std::string& key) const
	{
		auto it = settings.find(key);
		if (it == settings.end())
		{
			it = defaults.find(key);
			if (it == defaults.end()) return T();
		}
		const std::string& v = it->second;
		try
		{
			if constexpr (std::is_same_v<T, int>) return std::stoi(v);
			else if constexpr (std::is_same_v<T, float>) return std::stof(v);
			else if constexpr (std::is_same_v<T, bool>) return v == "true" || v == "1";
			else if constexpr (std::is_same_v<T, std::string>) return v;
			else return T();
		}
		catch (...)
		{
			return T();
		}
	}

	template <typename T>
	void setValue(const std::string& key, T value, const std::string& eventName = "")
	{
		if constexpr (std::is_same_v<T, bool>) settings[key] = value ? "true" : "false";
		else if constexpr (std::is_same_v<T, std::string>) settings[key] = value;
		else settings[key] = std::to_string(value);
		saveSettings();
		if (!eventName.empty()) EventBus::getInstance().emit<T>(eventName, value);
	}
};