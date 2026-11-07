#pragma once
#include "Utilities.h"
#include "AssetDatabase.h"
#include "IDrawer.h"
#include "IAudioBackend.h"

struct SoundResource
{
	int handle = -1;
};

/// <summary>
/// assets のファイルを描画・音声のハンドルにしてキャッシュする。読めなかった ID も覚えて毎フレーム読み直さない
/// </summary>
class ResourceManager
{
private:
	ResourceManager() = default;
	std::map<AssetID, ImageResource> images;
	std::map<AssetID, SoundResource> sounds;
	std::map<AssetID, int> meshes;
	std::map<AssetID, std::vector<unsigned char>> soundData; //音声バックエンドが参照し続けるので持っておく
public:
	static ResourceManager& getInstance()
	{
		static ResourceManager instance;
		return instance;
	}
	ImageResource loadImage(IDrawer& drawer, const AssetID& assetID);
	SoundResource loadSound(IAudioBackend& backend, const AssetID& assetID);
	/// <summary>
	/// "builtin:cube" などの基本形か、.gltf / .glb のモデル。読めなければ -1
	/// </summary>
	int loadMesh(IDrawer& drawer, const AssetID& assetID);
	void unloadAll();
};
