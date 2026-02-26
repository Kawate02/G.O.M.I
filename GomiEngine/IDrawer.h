#pragma once
#include <string>
#include "TextAnchor.h"
/// <summary>
/// 描画システムの抽象レイヤー
/// </summary>
class IDrawer
{
public:
    virtual void init() = 0;
	virtual void clearScreen() = 0;
    virtual void drawImage(int handle, int x, int y, int w, int h, int sx, int sy, float angle) = 0;
    virtual void drawText(const std::string& text, int fontSize, Anchor anchor, int x, int y) = 0;
    virtual void present() = 0;
    virtual ~IDrawer() = default;
};