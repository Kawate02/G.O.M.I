#pragma once

#include <string>
#include <vector>
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
};