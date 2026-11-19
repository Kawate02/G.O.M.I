#pragma once
#include "include/json.hpp"
#include "EnumNames.h"
#include "Vec2.h"
#include "Vec3.h"

#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

using Json = nlohmann::json;

/// <summary>
/// std::unordered_map / std::map(JSON ではキーを文字列にしたオブジェクト)
/// </summary>
template <typename T>
struct IsJsonMap : std::false_type {};
template <typename K, typename V, typename H, typename E, typename A>
struct IsJsonMap<std::unordered_map<K, V, H, E, A>> : std::true_type {};
template <typename K, typename V, typename C, typename A>
struct IsJsonMap<std::map<K, V, C, A>> : std::true_type {};

/// <summary>
/// std::vector(JSON では配列)
/// </summary>
template <typename T>
struct IsJsonVector : std::false_type {};
template <typename V, typename A>
struct IsJsonVector<std::vector<V, A>> : std::true_type {};

/// <summary>
/// GOMI_FIELDS で項目を並べた構造体(JSON では項目名のオブジェクト)
/// </summary>
template <typename T>
concept FieldStruct = std::is_class_v<T> && requires { gomiFieldList(static_cast<const T*>(nullptr)); };

// ---- GOMI_FIELDS(型, 項目, 項目, ...): 入れ子の構造体の項目を並べる(最大32項目)
#define GOMI_FE_EXPAND(x) x
#define GOMI_FE_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,N,...) N
#define GOMI_FE_COUNT(...) GOMI_FE_EXPAND(GOMI_FE_ARG_N(__VA_ARGS__,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1))
#define GOMI_FE_CAT_IMPL(a, b) a##b
#define GOMI_FE_CAT(a, b) GOMI_FE_CAT_IMPL(a, b)
#define GOMI_FE_1(T, m) std::pair{ #m, &T::m }
#define GOMI_FE_2(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_1(T, __VA_ARGS__))
#define GOMI_FE_3(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_2(T, __VA_ARGS__))
#define GOMI_FE_4(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_3(T, __VA_ARGS__))
#define GOMI_FE_5(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_4(T, __VA_ARGS__))
#define GOMI_FE_6(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_5(T, __VA_ARGS__))
#define GOMI_FE_7(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_6(T, __VA_ARGS__))
#define GOMI_FE_8(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_7(T, __VA_ARGS__))
#define GOMI_FE_9(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_8(T, __VA_ARGS__))
#define GOMI_FE_10(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_9(T, __VA_ARGS__))
#define GOMI_FE_11(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_10(T, __VA_ARGS__))
#define GOMI_FE_12(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_11(T, __VA_ARGS__))
#define GOMI_FE_13(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_12(T, __VA_ARGS__))
#define GOMI_FE_14(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_13(T, __VA_ARGS__))
#define GOMI_FE_15(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_14(T, __VA_ARGS__))
#define GOMI_FE_16(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_15(T, __VA_ARGS__))
#define GOMI_FE_17(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_16(T, __VA_ARGS__))
#define GOMI_FE_18(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_17(T, __VA_ARGS__))
#define GOMI_FE_19(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_18(T, __VA_ARGS__))
#define GOMI_FE_20(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_19(T, __VA_ARGS__))
#define GOMI_FE_21(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_20(T, __VA_ARGS__))
#define GOMI_FE_22(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_21(T, __VA_ARGS__))
#define GOMI_FE_23(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_22(T, __VA_ARGS__))
#define GOMI_FE_24(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_23(T, __VA_ARGS__))
#define GOMI_FE_25(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_24(T, __VA_ARGS__))
#define GOMI_FE_26(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_25(T, __VA_ARGS__))
#define GOMI_FE_27(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_26(T, __VA_ARGS__))
#define GOMI_FE_28(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_27(T, __VA_ARGS__))
#define GOMI_FE_29(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_28(T, __VA_ARGS__))
#define GOMI_FE_30(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_29(T, __VA_ARGS__))
#define GOMI_FE_31(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_30(T, __VA_ARGS__))
#define GOMI_FE_32(T, m, ...) GOMI_FE_1(T, m), GOMI_FE_EXPAND(GOMI_FE_31(T, __VA_ARGS__))

/// <summary>
/// 構造体の直後(同じ名前空間)に書くと、その構造体を Component・Resource の項目の中で使える(JSON では項目名のオブジェクト)。
///   struct Item { std::string id; int count = 0; };
///   GOMI_FIELDS(Item, id, count)
/// 書き忘れるとビルドで「toJson: unsupported type」になる
/// </summary>
#define GOMI_FIELDS(T, ...) 	[[maybe_unused]] inline auto gomiFieldList(const T*) 	{ 		return std::make_tuple(GOMI_FE_EXPAND(GOMI_FE_CAT(GOMI_FE_, GOMI_FE_COUNT(__VA_ARGS__))(T, __VA_ARGS__))); 	}

/// <summary>
/// float を「0.6」のような短い表記の double にする(そのままだと 0.6000000238... と保存される)
/// </summary>
inline double floatToJsonNumber(float f)
{
	char buf[32];
	auto result = std::to_chars(buf, buf + sizeof(buf) - 1, f);
	*result.ptr = '\0';
	return std::strtod(buf, nullptr);
}

/// <summary>
/// コンポーネントの項目を JSON にする。
/// 数値・bool・文字列はそのまま、Vec2/Vec3 は配列、enum は名前、ビット enum は名前の配列、
/// map はキーを文字列にしたオブジェクト、vector は配列、GOMI_FIELDS の構造体は項目名のオブジェクト
/// </summary>
template <typename T>
Json toJson(const T& v)
{
	if constexpr (std::is_same_v<T, float>)
	{
		return Json(floatToJsonNumber(v));
	}
	else if constexpr (std::is_same_v<T, bool> || std::is_arithmetic_v<T> || std::is_same_v<T, std::string>)
	{
		return Json(v);
	}
	else if constexpr (std::is_same_v<T, Vec2>)
	{
		return Json::array({ floatToJsonNumber(v.x), floatToJsonNumber(v.y) });
	}
	else if constexpr (std::is_same_v<T, Vec3>)
	{
		return Json::array({ floatToJsonNumber(v.x), floatToJsonNumber(v.y), floatToJsonNumber(v.z) });
	}
	else if constexpr (NamedEnum<T>)
	{
		if constexpr (IsFlagEnum<T>::value)
		{
			using U = std::underlying_type_t<T>;
			U bits = static_cast<U>(v);
			Json names = Json::array();
			for (auto& [e, name] : EnumNames<T>::list())
			{
				U eb = static_cast<U>(e);
				if (eb != 0 && (bits & eb) == eb) names.push_back(name);
			}
			return names;
		}
		else
		{
			const char* name = enumToName(v);
			if (!name) throw std::runtime_error("enum value " + std::to_string(static_cast<long long>(v)) + " has no name");
			return name;
		}
	}
	else if constexpr (IsJsonVector<T>::value)
	{
		Json arr = Json::array();
		for (auto&& item : v) arr.push_back(toJson(static_cast<const typename T::value_type&>(item)));
		return arr;
	}
	else if constexpr (FieldStruct<T>)
	{
		Json obj = Json::object();
		std::apply([&](auto&&... f) { ((obj[f.first] = toJson(v.*(f.second))), ...); }, gomiFieldList(static_cast<const T*>(nullptr)));
		return obj;
	}
	else if constexpr (IsJsonMap<T>::value)
	{
		Json obj = Json::object();
		for (auto& [key, value] : v)
		{
			Json k = toJson(key);
			if (!k.is_string()) throw std::runtime_error("map key must be a string or a named enum");
			obj[k.template get<std::string>()] = toJson(value);
		}
		return obj;
	}
	else
	{
		static_assert(sizeof(T) == 0, "toJson: unsupported type");
	}
}

template <typename T>
void fromJson(const Json& j, T& v)
{
	if constexpr (std::is_same_v<T, bool> || std::is_arithmetic_v<T> || std::is_same_v<T, std::string>)
	{
		v = j.template get<T>();
	}
	else if constexpr (std::is_same_v<T, Vec2>)
	{
		if (!j.is_array() || j.size() != 2) throw std::runtime_error("expected [x, y]");
		v = Vec2(j[0].template get<float>(), j[1].template get<float>());
	}
	else if constexpr (std::is_same_v<T, Vec3>)
	{
		//2D 用に [x, y] も受け付ける(z = 0)
		if (!j.is_array() || (j.size() != 3 && j.size() != 2)) throw std::runtime_error("expected [x, y, z]");
		v = Vec3(j[0].template get<float>(), j[1].template get<float>(), j.size() == 3 ? j[2].template get<float>() : 0.0f);
	}
	else if constexpr (NamedEnum<T>)
	{
		auto parse = [](const Json& s) {
			if (!s.is_string()) throw std::runtime_error("expected an enum name (string)");
			auto e = enumFromName<T>(s.template get<std::string>());
			if (!e) throw std::runtime_error("unknown name \"" + s.template get<std::string>() + "\"");
			return *e;
		};
		if constexpr (IsFlagEnum<T>::value)
		{
			using U = std::underlying_type_t<T>;
			if (!j.is_array()) throw std::runtime_error("expected an array of names");
			U bits = 0;
			for (auto& s : j) bits |= static_cast<U>(parse(s));
			v = static_cast<T>(bits);
		}
		else
		{
			v = parse(j);
		}
	}
	else if constexpr (IsJsonVector<T>::value)
	{
		if (!j.is_array()) throw std::runtime_error("expected an array");
		v.clear();
		for (size_t i = 0; i < j.size(); i++)
		{
			typename T::value_type item{};
			try { fromJson(j[i], item); }
			catch (const std::exception& ex) { throw std::runtime_error("[" + std::to_string(i) + "]: " + ex.what()); }
			v.push_back(std::move(item));
		}
	}
	else if constexpr (FieldStruct<T>)
	{
		//書いた項目だけ上書きする(書かない項目は今の値のまま)。知らない項目名は例外
		if (!j.is_object()) throw std::runtime_error("expected an object");
		auto fields = gomiFieldList(static_cast<const T*>(nullptr));
		for (auto& [key, value] : j.items())
		{
			bool found = false;
			auto tryField = [&](const auto& f) {
				if (found || key != f.first) return;
				found = true;
				try { fromJson(value, v.*(f.second)); }
				catch (const std::exception& ex) { throw std::runtime_error(key + ": " + ex.what()); }
			};
			std::apply([&](const auto&... f) { (tryField(f), ...); }, fields);
			if (!found) throw std::runtime_error("unknown field \"" + key + "\"");
		}
	}
	else if constexpr (IsJsonMap<T>::value)
	{
		if (!j.is_object()) throw std::runtime_error("expected an object");
		v.clear();
		for (auto& [key, value] : j.items())
		{
			typename T::key_type k;
			typename T::mapped_type m;
			fromJson(Json(key), k);
			try { fromJson(value, m); }
			catch (const std::exception& ex) { throw std::runtime_error(key + ": " + ex.what()); }
			v[k] = m;
		}
	}
	else
	{
		static_assert(sizeof(T) == 0, "fromJson: unsupported type");
	}
}
