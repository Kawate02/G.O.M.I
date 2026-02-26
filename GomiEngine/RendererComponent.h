#pragma once

#include <string>
struct RendererComponent
{
	std::string path;
	int layer = 0;
	int order = 0;
	bool enable = true;
	bool useCamera = true;
};