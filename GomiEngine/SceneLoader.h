#pragma once
#include "JsonConvert.h"
#include "AssetDatabase.h"
#include "UIManager.h"

#include <string>
#include <vector>

class World;

/// <summary>
/// シーン・プレハブ・UI の JSON ファイルを World に読み込む/World から書き出す。
///
/// シーン(scenes/*.json):
///   { "systems": ["RendererSystem", ...],
///     "entities": [ { "prefab": "prefabs/player.json", "overrides": { "Transform": { "position": [100, 300, 0] } } },
///                   { "components": { "Transform": { ... }, ... } } ],
///     "ui": [ { "prefab": "ui/title.json", "pos": [0, 0], "layer": 0 } ] }
/// プレハブ(prefabs/*.json): { "entities": [ { "components": { ... } }, ... ] }
///   overrides はプレハブの最初の Entity のコンポーネントに項目単位で重ねる
/// UI(ui/*.json): { "widgets": [ { "type": "Text" | "Image" | "Button", ... }, ... ] }
/// </summary>
namespace SceneLoader
{
	/// <summary>
	/// "Field" → "scenes/Field.json"。.json で終わる時はそのままアセット ID とみなす
	/// </summary>
	AssetID scenePath(const std::string& nameOrPath);

	/// <summary>
	/// シーンの JSON を空の World に読み込む。間違いがあれば場所の分かる例外
	/// </summary>
	void load(World& world, const Json& scene, const std::string& source);

	/// <summary>
	/// World の今の状態をシーンの JSON にする(Entity はプレハブを展開した components で書く)
	/// </summary>
	Json save(World& world);

	/// <summary>
	/// UI の JSON から部品を作り、まとめて動かせる親のハンドルを返す
	/// </summary>
	UIHandle spawnUI(UIManager& ui, const Json& prefab, Vec2 pos, int layer, const std::string& source);

	/// <summary>
	/// base に patch を重ねる(オブジェクトは中まで、それ以外は置き換え)
	/// </summary>
	Json merge(Json base, const Json& patch);
}
