#pragma once
#include "include/json.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/// <summary>
/// Entity に付けるタグの組(1タグ = 1ビット)。どのビットがどの名前かはプロジェクトの project.json の "tags" で決まる(TagRegistry)。
/// JSON では名前の配列(["UNIT", "PLAYER"])。ゲームのコードでは TagSet::of("ITEM") で名前から作る
/// </summary>
struct TagSet
{
	uint32_t bits = 0;

	/// <summary>
	/// 名前から作る。project.json の "tags" に無い名前は例外
	/// </summary>
	static TagSet of(std::string_view name);
	inline bool has(TagSet t) const { return (bits & t.bits) != 0; }
	inline bool empty() const { return bits == 0; }
	inline TagSet operator|(TagSet o) const { return { bits | o.bits }; }
	bool operator==(const TagSet&) const = default;
};

/// <summary>
/// タグの名前の並び。project.json の "tags": ["UNIT", "WALL", ...](並んだ順にビット 0, 1, ... を割り当てる。32個まで)
/// </summary>
class TagRegistry
{
private:
	std::vector<std::string> names;
	TagRegistry() = default;
public:
	static constexpr size_t MAX_TAGS = 32;
	// DLL をまたいで1つにするため、実体は .cpp に置く
	static TagRegistry& getInstance();

	/// <summary>
	/// 名前の並びを置き換える。空の名前・重なり・33個以上は例外(その時は今のまま)
	/// </summary>
	void set(std::vector<std::string> list);
	/// <summary>
	/// project.json の中身から "tags" を読む(無ければタグ無し)。間違いは "project.json: tags: ..." の例外
	/// </summary>
	void loadProject(const nlohmann::json& project);

	inline const std::vector<std::string>& getNames() const { return names; }
	std::optional<TagSet> find(std::string_view name) const;
	/// <summary>
	/// 名前のタグ。無い名前は「project.json の tags に足す」と書いた例外
	/// </summary>
	TagSet get(std::string_view name) const;
	/// <summary>
	/// 立っているビットの名前(並びの順)。名前の無いビットは出さない
	/// </summary>
	std::vector<std::string> toNames(TagSet tags) const;
};
