#pragma once
#include "AudioCommand.h"
#include <vector>

class AudioQueue
{
private:
	std::vector<AudioCommand> commands;
public:
	void addCommand(const AudioCommand& command)
	{
		commands.push_back(command);
	}
	const std::vector<AudioCommand>& getCommands() const
	{
		return commands;
	}
	void clear()
	{
		commands.clear();
	}
};