#pragma once
#include "Vec3.h"

/// <summary>
/// 太陽のような平行光源。シーンに1つだけ効く(最初に見つかったもの)
/// </summary>
struct LightComponent
{
	Vec3 direction = { -0.4f, -1.0f, -0.3f }; //光が進む向き
	Vec3 color = { 1, 1, 1 };
	float intensity = 1;
	float ambient = 0.25f; //影になった面の明るさ
};
