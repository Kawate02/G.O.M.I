#pragma once
#include "Vec3.h"

struct CameraFollowComponent
{
	Vec3 offset; //カメラがどれだけズレるか
	bool x = true; //xを追従
	bool y = true; //yを追従
	bool z = true; //zを追従(3D のカメラだけ)
};