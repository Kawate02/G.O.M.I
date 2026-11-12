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
    std::optional<MeshData> data = MeshFactory::builtin(assetID);
    if (data)
    {
        handle = drawer.createMesh(*data);
    }
    else if (ModelLoader::isModelFile(assetID))
    {
        auto& db = AssetDatabase::getInstance();
        std::string error = "asset not found";
        if (db.exists(assetID)) data = ModelLoader::loadGltf(db.resolve(assetID), error);
        if (data) handle = drawer.createMesh(*data);
        else db.warnOnce(assetID, error);
    }
    else if (!assetID.empty())
    {
        AssetDatabase::getInstance().warnOnce(assetID, "unknown mesh");
    }
    meshes[assetID] = handle;
    if (data && handle != -1)
    {
        glm::vec3 lo(1e30f), hi(-1e30f);
        for (size_t i = 0; i + 2 < data->vertices.size(); i += MeshData::STRIDE)
        {
            glm::vec3 p(data->vertices[i], data->vertices[i + 1], data->vertices[i + 2]);
            lo = glm::min(lo, p);
            hi = glm::max(hi, p);
        }
        meshBounds[assetID] = { lo, hi };
    }
    return handle;
}

bool ResourceManager::getMeshBounds(const AssetID& assetID, glm::vec3& min, glm::vec3& max) const
{
    auto it = meshBounds.find(assetID);
    if (it == meshBounds.end()) return false;
    min = it->second.first;
    max = it->second.second;
    return true;
}

void ResourceManager::unloadAll()
{
    images.clear();
    sounds.clear();
    meshes.clear();
}

ResourceManager& ResourceManager::getInstance()
{
	static ResourceManager instance;
	return instance;
}
