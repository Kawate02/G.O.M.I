#pragma once
#include "EnumNames.h"

#include <cstdint>

/// <summary>
/// CLAMP: 画面(カメラの範囲)の中に押し戻す。DESPAWN: 画面の外に出たら消す
/// </summary>
enum class OffScreenMode : uint8_t
{
	CLAMP,
	DESPAWN
};
GOMI_ENUM_NAMES(OffScreenMode,
	GOMI_ENUM_ENTRY(OffScreenMode, CLAMP),
	GOMI_ENUM_ENTRY(OffScreenMode, DESPAWN))

/// <summary>
/// 画面の外に出た時の扱い(OffScreenSystem が見る。2D 用。大きさは Renderer の画像から取る)
/// </summary>
struct OffScreenComponent
{
	OffScreenMode mode = OffScreenMode::CLAMP;
};
