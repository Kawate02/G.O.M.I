#include "AssetTable.h"

const AssetTable assetTable = {
	{"", {}},
	{"player", {AssetType::IMAGE_ASSET, IDR_PLAYER_PNG, {64, 64}}},
	{"background", {AssetType::IMAGE_ASSET, IDR_BG_PNG, {1600, 600}}},
	{"middleground", {AssetType::IMAGE_ASSET, IDR_MG_PNG, {1600, 600}}},
	{"foreground", {AssetType::IMAGE_ASSET, IDR_FG_PNG, {1600, 600}}},
	{"ag", {AssetType::IMAGE_ASSET, IDR_AG_PNG, {1600, 600}}},
	{"screen", {AssetType::IMAGE_ASSET, IDR_SCREEN_PNG, {800, 600}}},
	{"ui_frame", {AssetType::IMAGE_ASSET, IDR_UI_FRAME_PNG, {64, 64}}},
	{"ui_hover", {AssetType::IMAGE_ASSET, IDR_UI_HOVER_PNG, {64, 28}}},
	{"cursor_move", {AssetType::AUDIO_ASSET, IDR_CURSOR_MOVE_WAV, {}}},
	{"cursor_click", {AssetType::AUDIO_ASSET, IDR_CURSOR_CLICK_WAV, {}}},
};