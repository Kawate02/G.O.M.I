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
	std::map<AssetID, std::pair<glm::vec3, glm::vec3>> meshBounds;
	std::map<AssetID, std::vector<unsigned char>> soundData; //音声バックエンドが参照し続けるので持っておく
public:
	// DLL をまたいで1つにするため、実体は .cpp に置く(ヘッダの inline だとモジュールごとに別物になる)
	static ResourceManager& getInstance();
	ImageResource loadImage(IDrawer& drawer, const AssetID& assetID);
	SoundResource loadSound(IAudioBackend& backend, const AssetID& assetID);
	/// <summary>
	/// "builtin:cube" などの基本形か、.gltf / .glb のモデル。読めなければ -1
	/// </summary>
	int loadMesh(IDrawer& drawer, const AssetID& assetID);
	/// <summary>
	/// 読み込み済みメッシュの範囲(モデルの座標で min, max)。まだ読んでいなければ false
	/// </summary>
	bool getMeshBounds(const AssetID& assetID, glm::vec3& min, glm::vec3& max) const;
	void unloadAll();
};
