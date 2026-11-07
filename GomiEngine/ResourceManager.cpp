#include "ResourceManager.h"

ImageResource ResourceManager::loadImage(IDrawer& drawer, const AssetID& assetID)
{
    auto cached = images.find(assetID);
    if (cached != images.end()) return cached->second;

    auto& db = AssetDatabase::getInstance();
    ImageResource res;
    if (db.exists(assetID))
    {
        auto bytes = db.readBytes(assetID);
        res = drawer.createTexture(bytes.data(), bytes.size());
        if (res.handle == -1) db.warnOnce(assetID, "cannot decode image");
    }
    else if (!assetID.empty())
    {
        db.warnOnce(assetID, "asset not found");
    }
    images[assetID] = res;
    return res;
}

SoundResource ResourceManager::loadSound(IAudioBackend& backend, const AssetID& assetID)
{
    auto cached = sounds.find(assetID);
    if (cached != sounds.end()) return cached->second;

    auto& db = AssetDatabase::getInstance();
    SoundResource res;
    if (db.exists(assetID))
    {
        auto& data = soundData[assetID];
        data = db.readBytes(assetID);
        res.handle = backend.load(assetID, data.data(), data.size());
        if (res.handle == -1) db.warnOnce(assetID, "cannot decode sound");
    }
    else if (!assetID.empty())
    {
        db.warnOnce(assetID, "asset not found");
    }
    sounds[assetID] = res;
    return res;
}

void ResourceManager::unloadAll()
{
    images.clear();
    sounds.clear();
}
