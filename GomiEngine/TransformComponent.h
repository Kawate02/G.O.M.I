#pragma once
#include "Vec3.h"

struct TransformComponent
{
	Vec3 position;
	Vec3 scale;
	Vec3 rotation; //度。x: ピッチ, y: ヨー, z: ロール(2D の回転は z)
};