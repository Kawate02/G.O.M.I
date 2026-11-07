#pragma once

#include <unordered_map>
#include "Action.h"
#include "KeyCode.h"
#include "EnumNames.h"

/// <summary>
/// 上下左右の入力をどの平面で動かすか。XY は 2D(上が -y)、XZ は 3D の地面(上が奥 -z)
/// </summary>
enum class MovePlane : uint8_t
{
	XY,
	XZ
};
GOMI_ENUM_NAMES(MovePlane,
	GOMI_ENUM_ENTRY(MovePlane, XY),
	GOMI_ENUM_ENTRY(MovePlane, XZ))

struct PlayerControlComponent
{
	std::unordered_map<ActionID, InputKey> actions;
	MovePlane plane = MovePlane::XY;
	float jumpSpeed = 800; //ジャンプの初速(重力と逆向き)
	float dashSpeed = 1000; //ダッシュで +x に足す速さ
};