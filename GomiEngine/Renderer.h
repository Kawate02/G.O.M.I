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
	/// キューをまとめて描画(画面の切り替え present は呼び出し側で行う)
	/// </summary>
	/// <param name="drawer"></param>
	/// <param name="queue"></param>
	void render(IDrawer& drawer, RenderQueue& queue, const Camera& camera);
};
