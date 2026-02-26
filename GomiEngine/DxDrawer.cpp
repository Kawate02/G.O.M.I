#include "DxDrawer.h"

void DxDrawer::drawImage(int handle, int x, int y, int w, int h, int sx, int sy, float angle)
{
	if (sx == sy)
	{
		DrawRotaGraph(x + (w * sx) / 2, y + (h * sy) / 2, sx, angle, handle, TRUE);
	}
	else
	{
		DrawExtendGraph(x, y, x + (w * sx), y + (h * sy), handle, TRUE);
	}
}

void DxDrawer::drawText(const std::string& text, int fontSize, Anchor anchor, int x, int y)
{
	SetFontSize(fontSize);
	int w = GetDrawStringWidth(text.c_str(), text.length());
	int h = GetFontSize();
	switch (anchor)
	{
	case Anchor::TOP_LEFT:
		break;
	case Anchor::TOP_CENTER:
		x -= w / 2;
		break;
	case Anchor::TOP_RIGHT:
		x -= w;
		break;
	case Anchor::CENTER_LEFT:
		y -= h / 2;
		break;
	case Anchor::CENTER:
		x -= w / 2;
		y -= h / 2;
		break;
	case Anchor::CENTER_RIGHT:
		x -= w;
		y -= h / 2;
		break;
	case Anchor::BOTTOM_LEFT:
		y -= h;
		break;
	}
	DrawString(x, y, text.c_str(), GetColor(255, 255, 255));
}
