#pragma once

#include <string>
#include <vector>
struct Vec2;
struct RendererComponent
{
	std::string path;
	int layer = 0;
	int order = 0;
	bool enable = true;
	bool useCamera = true;
	/// <summary>画像の切り出す範囲 [u0, v0, u1, v1](0〜1)。既定は全体。SpriteAnimation が書く</summary>
	std::vector<float> uv = { 0, 0, 1, 1 };
	bool flipX = false; //左右反転
	/// <summary>掛ける色 [r, g, b, a](0〜1)。既定は白(変わらない)</summary>
	std::vector<float> color = { 1, 1, 1, 1 };
	/// <summary>塗りつぶす色 [r, g, b](0〜1)。flash の強さで絵の色から寄せる(白く光らせる)</summary>
	std::vector<float> flashColor = { 1, 1, 1 };
	/// <summary>塗りつぶす強さ 0〜1。既定 0(何もしない)</summary>
	float flash = 0;
};

/// <summary>
/// 画像の切り出し(uv)を含めた、scale をかける前の描く大きさ。読めない画像は {0, 0}
/// </summary>
Vec2 rendererImageSize(const RendererComponent& renderer);
