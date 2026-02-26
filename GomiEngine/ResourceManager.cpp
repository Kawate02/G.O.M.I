#include "ResourceManager.h"

ResourceBlob ResourceManager::WinLoadResource(int id)
{
    ResourceBlob blob{};

    HMODULE hModule = GetModuleHandle(nullptr);
    HRSRC res = FindResource(hModule, MAKEINTRESOURCE(id), RT_RCDATA);
    if (!res) return blob;

    HGLOBAL h = LoadResource(nullptr, res);
    if (!h) return blob;

    blob.data = LockResource(h);
    blob.size = SizeofResource(nullptr, res);
    return blob;
}

ImageResource ResourceManager::loadImage(const AssetID& assetID)
{
    auto it = assetTable.find(assetID);
	if (it == assetTable.end()) return {};
    if (it->second.type != AssetType::IMAGE_ASSET) return {};

	if (imagehandles.find(assetID) != imagehandles.end()) return imagehandles[assetID];

    auto entry = it->second;
    ResourceBlob blob = WinLoadResource(entry.file);
    if (blob.data == nullptr) return {};
    int handle = CreateGraphFromMem(blob.data, blob.size);
    if (handle == -1) return {};
    ImageResource res = { .handle = handle };
    GetGraphSize(handle, &res.width, &res.height);
    imagehandles[assetID] = res;
    return res;
}

SoundResource ResourceManager::loadSound(const AssetID& assetID)
{
    auto it = assetTable.find(assetID);
    if (it == assetTable.end()) return {};
	if (it->second.type != AssetType::AUDIO_ASSET) return {};

    auto entry = it->second;
    ResourceBlob blob = WinLoadResource(entry.file);
	if (blob.data == nullptr) return {};
	int handle = LoadSoundMemByMemImage(blob.data, blob.size);
	if (handle == -1) return {};
	SoundResource res = { .handle = handle };
	return res;
}

void ResourceManager::unloadAll()
{
    imagehandles.clear();
}
