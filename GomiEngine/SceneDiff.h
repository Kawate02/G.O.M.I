#pragma once
#include "JsonConvert.h"

#include <string>
#include <vector>

/// <summary>
/// シーンの JSON 2つの違い1つ(Entity・コンポーネント・項目の単位)
/// </summary>
struct SceneChange
{
	enum class Kind
	{
		EntityAdded,
		EntityRemoved,
		ComponentAdded,
		ComponentRemoved,
		FieldChanged,
		SystemsChanged,
		UIChanged,
	};
	Kind kind = Kind::FieldChanged;
	std::string entity;    //Entity の呼び名(名前、無ければ「プレハブのファイル [番号]」。同じ名前は #2 から番号を付ける)
	int index = -1;        //新しい側で何番目の Entity か(消えた Entity は -1)
	std::string component;
	std::string field;
	Json before;           //項目・System の並び・UI の前の値
	Json after;
	/// <summary>
	/// 1行の文字(「player > Transform.position: [0,0,0] -> [1,0,0]」など)
	/// </summary>
	std::string text() const;
};

namespace SceneDiff
{
	/// <summary>
	/// シーンの JSON の Entity を、プレハブを展開し既定値を埋めた形の並びにする。
	/// [ { "label": 呼び名(エディタのヒエラルキーと同じ), "components": { ... } }, ... ](プレハブが読めない Entity は components が空)
	/// </summary>
	Json expand(const Json& scene);
	/// <summary>
	/// before から after への違い。Entity は呼び名で対応させ、合わなければ同じ並び順のものを同じ Entity とみなす
	/// </summary>
	std::vector<SceneChange> compare(const Json& before, const Json& after);
	/// <summary>
	/// Entity の呼び名(番号の前)。名前 > プレハブのファイル名(仲間が複数なら [番号]) > Camera3D・Light・メッシュ・画像の名前 > "Entity"
	/// </summary>
	std::string entityLabel(const Json& components, const std::string& prefab = "", int index = 0, int members = 1);
	/// <summary>
	/// 同じ呼び名の2つ目から " #2" " #3" を付ける
	/// </summary>
	std::vector<std::string> numbered(const std::vector<std::string>& labels);
	/// <summary>
	/// 値を1行で短く書く(長ければ途中で切る)
	/// </summary>
	std::string shortValue(const Json& value, size_t limit = 60);
}
