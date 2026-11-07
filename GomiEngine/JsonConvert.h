#pragma once
#include "include/json.hpp"
#include "EnumNames.h"
#include "Vec2.h"
#include "Vec3.h"

#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

using Json = nlohmann::json;

template <typename T>
struct IsUnorderedMap : std::false_type {};
template <typename K, typename V, typename H, typename E, typename A>
struct IsUnorderedMap<std::unordered_map<K, V, H, E, A>> : std::true_type {};

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
/// unordered_map はキーを文字列にしたオブジェクト
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
	else if constexpr (IsUnorderedMap<T>::value)
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
	else if constexpr (IsUnorderedMap<T>::value)
	{
		if (!j.is_object()) throw std::runtime_error("expected an object");
		v.clear();
		for (auto& [key, value] : j.items())
		{
			typename T::key_type k;
			typename T::mapped_type m;
			fromJson(Json(key), k);
			fromJson(value, m);
			v[k] = m;
		}
	}
	else
	{
		static_assert(sizeof(T) == 0, "fromJson: unsupported type");
	}
}
