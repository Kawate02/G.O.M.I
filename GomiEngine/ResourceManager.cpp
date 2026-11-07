#include "ResourceManager.h"
#include "MeshFactory.h"
#include "ModelLoader.h"

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

int ResourceManager::loadMesh(IDrawer& drawer, const AssetID& assetID)
{
    auto cached = meshes.find(assetID);
    if (cached != meshes.end()) return cached->second;

    int handle = -1;
    if (auto data = MeshFactory::builtin(assetID))
    {
        handle = drawer.createMesh(*data);
    }
    else if (ModelLoader::isModelFile(assetID))
    {
        auto& db = AssetDatabase::getInstance();
        std::string error = "asset not found";
        std::optional<MeshData> model;
        if (db.exists(assetID)) model = ModelLoader::loadGltf(db.resolve(assetID), error);
        if (model) handle = drawer.createMesh(*model);
        else db.warnOnce(assetID, error);
    }
    else if (!assetID.empty())
    {
        AssetDatabase::getInstance().warnOnce(assetID, "unknown mesh");
    }
    meshes[assetID] = handle;
    return handle;
}

void ResourceManager::unloadAll()
{
    images.clear();
    sounds.clear();
    meshes.clear();
}
