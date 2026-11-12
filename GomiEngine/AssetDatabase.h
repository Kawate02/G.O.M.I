#pragma once
#include "JsonConvert.h"
#include "Vec2.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// <summary>
/// アセットの ID。assets フォルダからの相対パスを / 区切りで書く(例: "images/player.png")
/// </summary>
using AssetID = std::string;

/// <summary>
/// assets フォルダのファイルを ID で読み書きする
/// </summary>
class AssetDatabase
{
private:
	std::filesystem::path root;
	std::unordered_map<AssetID, Vec2> sizeCache;
	std::unordered_set<AssetID> warned;
	AssetDatabase() = default;
public:
	// DLL をまたいで1つにするため、実体は .cpp に置く(ヘッダの inline だとモジュールごとに別物になる)
	static AssetDatabase& getInstance();

	/// <summary>
	/// 起動時の assets フォルダを決める。指定 > 実行ファイルの隣の assets > ソースの assets(開発中)
	/// </summary>
	static std::filesystem::path findRoot(const std::string& requested);

	void setRoot(const std::filesystem::path& path);
	inline const std::filesystem::path& getRoot() const { return root; }
	std::filesystem::path resolve(const AssetID& id) const;
	bool exists(const AssetID& id) const;

	/// <summary>
	/// ファイルの中身。無ければ例外
	/// </summary>
	std::vector<unsigned char> readBytes(const AssetID& id) const;
	/// <summary>
	/// JSON として読む。無い・壊れている時はファイル名入りの例外
	/// </summary>
	Json readJson(const AssetID& id) const;
	void writeJson(const AssetID& id, const Json& json) const;
	/// <summary>
	/// 画像の大きさをヘッダだけ読んで返す(GPU に載せる前に使える)。読めなければ {0, 0}
	/// </summary>
	Vec2 imageSize(const AssetID& id);
	/// <summary>
	/// 同じ ID の警告は1回だけ出す
	/// </summary>
	void warnOnce(const AssetID& id, const std::string& message);
};
