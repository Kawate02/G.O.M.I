#pragma once
#include "SettingManager.h"


class GameSettings
{
public:
	int windowWidth;
	int windowHeight;
	bool fullscreen;
	GameSettings();

private:
	void readManager();
	void subscribeValueChanged();
	void generateJson();
};