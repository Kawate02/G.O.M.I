#pragma once
#include "Utilities.h"
#include "AssetTable.h"
#include <DxLib.h>

struct ResourceBlob
{
	void* data;
	DWORD size;
};

struct ImageResource
{
	int handle = -1;
	int width = 0;
	int height = 0;
};

struct SoundResource
{
	int handle = -1;
};

class ResourceManager
{
private:
	ResourceBlob WinLoadResource(int id);
	ResourceManager() = default;
	std::map<AssetID, ImageResource> imagehandles;
public:
	static ResourceManager& getInstance()
	{
		static ResourceManager instance;
		return instance;
	}
	ImageResource loadImage(const AssetID& assetID);
	SoundResource loadSound(const AssetID& assetID);
	void unloadAll();
};