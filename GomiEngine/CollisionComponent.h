#pragma once
#include "EnumNames.h"
#include "Vec3.h"

enum class CollisionType
{
	BOX,
	SPHERE
};
GOMI_ENUM_NAMES(CollisionType,
	GOMI_ENUM_ENTRY(CollisionType, BOX),
	GOMI_ENUM_ENTRY(CollisionType, SPHERE))

/// <summary>
/// 当たり判定の形。BOX は Transform.position + offset を角にした size の箱、
/// SPHERE は Transform.position + offset を中心にした半径 radius の球。
/// 2D では z を 0 にすれば、厚さ 0 の軸は「重なっている」とみなされ平面の判定になる
/// </summary>
struct CollisionComponent
{
	CollisionType type = CollisionType::BOX;
	Vec3 size;
	float radius;
	Vec3 offset;
};
