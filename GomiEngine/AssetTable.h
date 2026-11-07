#pragma once
#include "Utilities.h"
#include "resource/resource.h"

using AssetID = std::string;
using ResourceID = int;

enum class AssetType : uint8_t
{
	IMAGE_ASSET,
	AUDIO_ASSET,
	FONT_ASSET,
	JSON_ASSET,
};

struct AssetEntry
{
	AssetType type;
	ResourceID file;
	Vec2 size;
};

using AssetTable = std::unordered_map<AssetID, AssetEntry>;

// 関数の中の static にして、他のグローバル(UIプレハブ等)の初期化より先に使われても大丈夫にする
const AssetTable& getAssetTable();
