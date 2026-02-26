#pragma once

#include "RenderCommand.h"

class RenderQueue
{
private:
	std::vector<RenderCommand> commands;
public:
	void addCommand(const RenderCommand& command)
	{
		commands.push_back(command);
	}

	const std::vector<RenderCommand>& getCommands() const
	{
		return commands;
	}

	void clear()
	{
		commands.clear();
	}
};