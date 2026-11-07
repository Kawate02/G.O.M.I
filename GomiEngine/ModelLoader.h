#pragma once
#include "IDrawer.h"
#include <filesystem>
#include <optional>
#include <string>

/// <summary>
/// glTF 2.0(.gltf / .glb)のモデルを1つのメッシュにまとめて読む。
/// シーンの全ノードのメッシュを、ノードの位置・回転・拡大を掛けて合わせる。
/// 法線が無ければ面から作る。マテリアル・アニメーション・スキンはまだ読まない
/// </summary>
namespace ModelLoader
{
	inline bool isModelFile(const std::string& id)
	{
		auto ends = [&](const char* ext) {
			std::string e = ext;
			return id.size() >= e.size() && id.compare(id.size() - e.size(), e.size(), e) == 0;
		};
		return ends(".gltf") || ends(".glb");
	}

	/// <summary>
	/// 読めなければ nullopt で、error に理由を入れる
	/// </summary>
	std::optional<MeshData> loadGltf(const std::filesystem::path& path, std::string& error);
}
