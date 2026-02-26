#pragma once
#include "IDrawer.h"
#include <DxLib.h>

class DxDrawer : public IDrawer 
{
public:
    inline void init() override
    {
        SetDrawScreen(DX_SCREEN_BACK);
    }
	inline void clearScreen() override
	{
		ClearDrawScreen();
	}
	void drawImage(int handle, int x, int y, int w, int h, int sx, int sy, float angle) override;
	void drawText(const std::string& text, int fontSize, Anchor anchor, int x, int y) override;
	inline void present() override
	{
		ScreenFlip();
	}
};