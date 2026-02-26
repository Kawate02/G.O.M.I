#pragma once
#include "Utilities.h"
#include "AssetTable.h"
#include "TextAnchor.h"
#include <variant>

struct RenderKey
{
	int layer = 0;
	int order = 0;
	bool useCamera = true;
};
struct SpriteRenderCommand
{
	AssetID assetID;
	Vec2 position;
	Vec2 scale = { 1,1 };
	float angle = 0;
	RenderKey key;
};

struct TextRenderCommand
{
	std::string text;
	Vec2 position;
	int fontSize;
	Anchor anchor;
	RenderKey key;
};

using RenderCommand = std::variant<SpriteRenderCommand, TextRenderCommand>;