#pragma once
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

/// <summary>
/// enum の値と名前の対応表。JSON への保存・読み込みやエディタの選択肢に使う。
/// 対応表を持たせる enum は GOMI_ENUM_NAMES で特殊化する
/// </summary>
template <typename E>
struct EnumNames;

/// <summary>
/// ビットを組み合わせて使う enum(EntityTag など)。JSON では名前の配列になる
/// </summary>
template <typename E>
struct IsFlagEnum : std::false_type {};

template <typename E>
concept NamedEnum = std::is_enum_v<E> && requires { EnumNames<E>::list(); };

template <NamedEnum E>
const char* enumToName(E value)
{
	for (auto& [v, name] : EnumNames<E>::list())
	{
		if (v == value) return name;
	}
	return nullptr;
}

template <NamedEnum E>
std::optional<E> enumFromName(std::string_view name)
{
	for (auto& [v, n] : EnumNames<E>::list())
	{
		if (name == n) return v;
	}
	return std::nullopt;
}

#define GOMI_ENUM_ENTRY(E, name) { E::name, #name }
#define GOMI_ENUM_NAMES(E, ...) \
	template <> struct EnumNames<E> \
	{ \
		static const std::vector<std::pair<E, const char*>>& list() \
		{ \
			static const std::vector<std::pair<E, const char*>> entries = { __VA_ARGS__ }; \
			return entries; \
		} \
	};
