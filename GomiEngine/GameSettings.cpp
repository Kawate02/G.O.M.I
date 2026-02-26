#include "GameSettings.h"

GameSettings::GameSettings()
{
	SettingManager::getInstance().loadSettings();
	//generateJson();
	readManager();
	subscribeValueChanged();
}

void GameSettings::readManager()
{
	auto& sm = SettingManager::getInstance();
	windowWidth = sm.getValue<int>("windowWidth");
	windowHeight = sm.getValue<int>("windowHeight");
	fullscreen = sm.getValue<bool>("fullscreen");
}

void GameSettings::subscribeValueChanged()
{
	auto& eb = EventBus::getInstance();
	eb.subscribe("windowSizeChange", [this](std::any value) {
		windowWidth = SettingManager::getInstance().getValue<int>("windowWidth");
		windowHeight = SettingManager::getInstance().getValue<int>("windowHeight");
		});
	eb.subscribe("fullscreenChange", [this](std::any value) {
		fullscreen = SettingManager::getInstance().getValue<bool>("fullscreen");
		});
}

void GameSettings::generateJson()
{
	auto& sm = SettingManager::getInstance();
	sm.setValue<int>("windowWidth", 800);
	sm.setValue<int>("windowHeight", 600);
	sm.setValue<bool>("fullscreen", false);
	sm.saveSettings();
}
