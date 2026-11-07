#pragma once
#include "Utilities.h"
#include "AssetTable.h"
#include "IDrawer.h"
#include "IAudioBackend.h"

struct ResourceBlob
{
	const void* data = nullptr;
	size_t size = 0;
};

struct SoundResource
{
	int handle = -1;
};

/// <summary>
/// exe に埋め込んだリソース(.rc)を読み、描画・音声のハンドルにしてキャッシュする
/// </summary>
class ResourceManager
{
private:
	ResourceBlob loadEmbedded(int id);
	ResourceManager() = default;
	std::map<AssetID, ImageResource> images;
	std::map<AssetID, SoundResource> sounds;
public:
	static ResourceManager& getInstance()
	{
		static ResourceManager instance;
		return instance;
	}
	ImageResource loadImage(IDrawer& drawer, const AssetID& assetID);
	SoundResource loadSound(IAudioBackend& backend, const AssetID& assetID);
	void unloadAll();
};
