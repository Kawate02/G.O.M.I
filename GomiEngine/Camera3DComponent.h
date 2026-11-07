#pragma once
#include "Vec3.h"

/// <summary>
/// 3D を見るカメラ。Transform の位置から、回転の向き(回転 0 で -Z)を見る。
/// active なものが複数あれば最初の1つを使う
/// </summary>
struct Camera3DComponent
{
	float fov = 60;          //縦の視野角(度)
	float nearClip = 0.1f;
	float farClip = 1000;
	Vec3 clearColor = { 0.45f, 0.6f, 0.8f }; //背景の色
	bool active = true;
};
