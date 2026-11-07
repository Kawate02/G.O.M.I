#pragma once
#include "Vec2.h"

struct CameraFollowComponent
{
	Vec2 offset; //カメラがどれだけズレるか
	bool x = true; //xを追従
	bool y = true; //yを追従
};