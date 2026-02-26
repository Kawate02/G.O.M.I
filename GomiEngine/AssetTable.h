#pragma once
#include "Utilities.h"
#include "resource/resource.h"
#include <windows.h>

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

const extern AssetTable assetTable;