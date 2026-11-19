#pragma once
#include "EnumNames.h"
#include "EntityTag.h"

#include <map>
#include <string>

using RuleID = std::string;
/// <summary>
/// BLOCK: 重なる手前で止める(MoveResolveSystem・PhysicsSystem)。TRIGGER: 止めずに、重なった両方に CollisionEvent を出す(CollisionSystem)
/// </summary>
enum class CollisionKind : uint8_t
{
	BLOCK,
	TRIGGER
};
GOMI_ENUM_NAMES(CollisionKind,
	GOMI_ENUM_ENTRY(CollisionKind, BLOCK),
	GOMI_ENUM_ENTRY(CollisionKind, TRIGGER))

/// <summary>
/// タグ a を持つ物とタグ b を持つ物が重なった時の扱い(a と b は入れ替えても同じ)
/// </summary>
struct CollisionRule
{
	TagSet a;
	TagSet b;
	CollisionKind kind = CollisionKind::BLOCK;
};

/// <summary>
/// 当たり判定のルールの表。プロジェクトの assets/collision_rules.json:
///   { "unit_to_wall": { "a": "UNIT", "b": "WALL", "kind": "BLOCK" }, ... }
/// ルールID の順(std::map)に回るので、毎回同じ順になる
/// </summary>
class CollisionRules
{
private:
	std::map<RuleID, CollisionRule> rules;
	CollisionRules() = default;
public:
	static constexpr const char* FILE_NAME = "collision_rules.json";
	// DLL をまたいで1つにするため、実体は .cpp に置く
	static CollisionRules& getInstance();

	inline const std::map<RuleID, CollisionRule>& all() const { return rules; }
	inline void set(std::map<RuleID, CollisionRule> table) { rules = std::move(table); }
	/// <summary>
	/// JSON からルールの表を作る(タグの名前は TagRegistry で引く)。間違いは "collision_rules.json: <ルールID>: ..." の例外
	/// </summary>
	static std::map<RuleID, CollisionRule> parse(const nlohmann::json& json);
	const CollisionRule* find(const RuleID& id) const;
};

/// <summary>
/// プロジェクトのタグ(project.json の "tags")と当たり判定のルール(assets/collision_rules.json。無ければルール無し)を読む。
/// シーンを読む前に呼ぶ。間違いはファイル名付きの例外で、その時はどちらも今のまま
/// </summary>
void loadCollisionSettings(const nlohmann::json& project);
