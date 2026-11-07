#pragma once
#include "IDrawer.h"
#include "RenderQueue.h"
#include "ResourceManager.h"
#include "Camera.h"

class Renderer
{
public:
	Renderer() {}
	void init();
	/// <summary>
	/// キューをまとめて描画
	/// </summary>
	/// <param name="drawer"></param>
	/// <param name="queue"></param>
	void render(IDrawer& drawer, RenderQueue& queue, const Camera& camera);
};