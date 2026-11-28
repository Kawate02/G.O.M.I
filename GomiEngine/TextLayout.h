#pragma once
// Text の複数行の計算。描画(GlDrawer)から離してあるのでテストできる。
// 文字は `\n` で行に割り、行の高さは fontSize の 1.2 倍。`\n` が無ければ 1 行で、幅と高さは従来と同じ。
#include <functional>
#include <string>
#include <vector>

struct TextLine
{
	std::string text;
	float width = 0;
};

struct TextLayout
{
	std::vector<TextLine> lines;
	float width = 0;  // 一番長い行の幅
	float height = 0; // fontSize + (行数 - 1) × lineHeight
	float lineHeight = 0;
};

// advance は 1 文字(ASCII の 32〜126 だけ渡す)の送り幅。それ以外の文字は幅に数えない
inline TextLayout layoutText(const std::string& text, int fontSize, const std::function<float(unsigned char)>& advance)
{
	TextLayout layout;
	layout.lineHeight = fontSize * 1.2f;
	layout.lines.push_back({});
	for (unsigned char c : text)
	{
		if (c == '\n') { layout.lines.push_back({}); continue; }
		TextLine& line = layout.lines.back();
		line.text.push_back(static_cast<char>(c));
		if (c >= 32 && c < 127) line.width += advance(c);
	}
	for (const TextLine& line : layout.lines)
		if (line.width > layout.width) layout.width = line.width;
	layout.height = fontSize + (layout.lines.size() - 1) * layout.lineHeight;
	return layout;
}
