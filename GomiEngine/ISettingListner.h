#pragma once
#include "GameSettings.h"

class ISettingListner
{
public:
	virtual void OnSettingChanged(const GameSettings& settings) = 0;
};