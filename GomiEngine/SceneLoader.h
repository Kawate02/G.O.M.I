#pragma once
#include "JsonConvert.h"
#include "AssetDatabase.h"
#include "UIManager.h"
#include "Entity.h"

#include <functional>
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
	/// World の今の状態をシーンの JSON にする。プレハブから作った Entity は "prefab" + 変えた項目だけの "overrides" で書く
	/// (仲間が消えた・2番目以降を変えた・コンポーネントを外した時は、つながりを切って components で書く)
	/// </summary>
	Json save(World& world);
	/// <summary>
	/// プレハブの中身(各 Entity の既定値で埋めたコンポーネント)を prefabs から取って書き出す。
	/// エディタは読み込んだ時のプレハブで overrides を作る(外でプレハブが変わっても、自分の変更だけを overrides にする)
	/// </summary>
	Json save(World& world, const std::function<std::vector<Json>(const AssetID&)>& prefabs);
	/// <summary>
	/// プレハブのファイルを読み、各 Entity のコンポーネントを既定値で埋めて返す。読めなければ空
	/// </summary>
	std::vector<Json> prefabComponents(const AssetID& prefab);

	/// <summary>
	/// プレハブを1つ置き、作った Entity をプレハブ内の順に返す
	/// </summary>
	std::vector<Entity> instantiate(World& world, const AssetID& prefabId, const Json& overrides = Json());

	/// <summary>
	/// Entity の全コンポーネントを JSON にする({ "Transform": {...}, ... })
	/// </summary>
	Json componentsOf(World& world, Entity e);
	/// <summary>
	/// コンポーネントの JSON を、書いていない項目を既定値で埋めた形にする
	/// </summary>
	Json normalize(const Json& components);
	/// <summary>
	/// base から current で変わった所だけを返す(オブジェクトは中まで比べる)
	/// </summary>
	Json diff(const Json& base, const Json& current);

	/// <summary>
	/// UI の JSON から部品を作り、まとめて動かせる親のハンドルを返す
	/// </summary>
	UIHandle spawnUI(UIManager& ui, const Json& prefab, Vec2 pos, int layer, const std::string& source);

	/// <summary>
	/// base に patch を重ねる(オブジェクトは中まで、それ以外は置き換え)
	/// </summary>
	Json merge(Json base, const Json& patch);
}
