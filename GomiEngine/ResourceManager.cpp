#include "ResourceManager.h"
#include <windows.h>

ResourceBlob ResourceManager::loadEmbedded(int id)
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

ImageResource ResourceManager::loadImage(IDrawer& drawer, const AssetID& assetID)
{
    auto cached = images.find(assetID);
    if (cached != images.end()) return cached->second;

    auto it = assetTable.find(assetID);
    if (it == assetTable.end()) return {};
    if (it->second.type != AssetType::IMAGE_ASSET) return {};

    ResourceBlob blob = loadEmbedded(it->second.file);
    if (blob.data == nullptr) return {};
    ImageResource res = drawer.createTexture(static_cast<const unsigned char*>(blob.data), blob.size);
    if (res.handle == -1) return {};
    images[assetID] = res;
    return res;
}

SoundResource ResourceManager::loadSound(IAudioBackend& backend, const AssetID& assetID)
{
    auto cached = sounds.find(assetID);
    if (cached != sounds.end()) return cached->second;

    auto it = assetTable.find(assetID);
    if (it == assetTable.end()) return {};
    if (it->second.type != AssetType::AUDIO_ASSET) return {};

    ResourceBlob blob = loadEmbedded(it->second.file);
    if (blob.data == nullptr) return {};
    int handle = backend.load(assetID, blob.data, blob.size);
    if (handle == -1) return {};
    SoundResource res = { .handle = handle };
    sounds[assetID] = res;
    return res;
}

void ResourceManager::unloadAll()
{
    images.clear();
    sounds.clear();
}
