#pragma once
#include "EntityTag.h"

/// <summary>
/// Entity のタグ。名前は project.json の "tags" で決める(JSON では ["UNIT", "PLAYER"])
/// </summary>
struct EntityTagComponent
{
	TagSet tags;
	inline bool has(TagSet t) const { return tags.has(t); }
	/// <summary>
	/// 名前で調べる(project.json の "tags" に無い名前は例外)
	/// </summary>
	inline bool has(std::string_view name) const { return tags.has(TagSet::of(name)); }
	inline void set(TagSet t) { tags = tags | t; }
};
