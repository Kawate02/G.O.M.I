#pragma once
#include "Utilities.h"
#include "AssetDatabase.h"
#include "TextAnchor.h"
#include "MathUtil.h"
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

/// <summary>
/// 2D(画面座標)の描画命令。layer → order の順に重ねる
/// </summary>
using RenderCommand = std::variant<SpriteRenderCommand, TextRenderCommand>;

/// <summary>
/// 3D のメッシュ1つ分の描画命令
/// </summary>
struct MeshRenderCommand
{
	AssetID mesh;
	AssetID texture; //空なら白
	glm::mat4 model{ 1.0f };
	Vec3 color = { 1, 1, 1 };
};

/// <summary>
/// 3D を見るカメラ(このフレームで使うもの)
/// </summary>
struct Camera3DView
{
	glm::mat4 view{ 1.0f };
	float fov = 60;
	float nearClip = 0.1f;
	float farClip = 1000;
	Vec3 clearColor;
};

/// <summary>
/// 平行光源。direction は光が進む向き
/// </summary>
struct LightData
{
	Vec3 direction = { -0.4f, -1.0f, -0.3f };
	Vec3 color = { 1, 1, 1 };
	float intensity = 1;
	float ambient = 0.25f;
};
