#pragma once
#include <string>

using AudioID = std::string;
struct AudioCommand
{
	AudioID audioID;
	float volume = 1.0f;
};