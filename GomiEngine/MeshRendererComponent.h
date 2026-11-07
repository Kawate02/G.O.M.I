#pragma once
#include <string>
#include "Vec3.h"

/// <summary>
/// 3D のメッシュを Transform の位置・回転・拡大で描く
/// </summary>
struct MeshRendererComponent
{
	std::string mesh = "builtin:cube"; //"builtin:cube" / "builtin:plane" / "builtin:sphere" か、models/ の .gltf / .glb
	std::string texture;               //空ならテクスチャ無し(color だけ)
	Vec3 color = { 1, 1, 1 };          //テクスチャに掛ける色(0〜1)
	bool enable = true;
};
