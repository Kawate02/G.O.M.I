#pragma once
#include "JsonConvert.h"
#include "ComponentManager.h"

#include <functional>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

/// <summary>
/// エディタで項目を出す時の種類
/// </summary>
enum class FieldKind
{
	Bool,
	Int,
	Float,
	String,
	Vec2,
	Vec3,
	Enum,
	Flags,
	Other,
};

template <typename T>
constexpr FieldKind fieldKindOf()
{
	if constexpr (std::is_same_v<T, bool>) return FieldKind::Bool;
	else if constexpr (std::is_integral_v<T>) return FieldKind::Int;
	else if constexpr (std::is_floating_point_v<T>) return FieldKind::Float;
	else if constexpr (std::is_same_v<T, std::string>) return FieldKind::String;
	else if constexpr (std::is_same_v<T, Vec2>) return FieldKind::Vec2;
	else if constexpr (std::is_same_v<T, Vec3>) return FieldKind::Vec3;
	else if constexpr (NamedEnum<T>) return IsFlagEnum<T>::value ? FieldKind::Flags : FieldKind::Enum;
	else return FieldKind::Other;
}

/// <summary>
/// コンポーネントの1項目。component はそのコンポーネント型の実体を指す void*
/// </summary>
struct FieldInfo
{
	std::string name;
	FieldKind kind;
	std::vector<std::string> enumNames; //Enum / Flags の選択肢
	bool runtime = false; //ゲームが動いている間に変わる値(速度・接地など)。エディタでは読むだけにする
	std::function<Json(const void* component)> save;
	std::function<void(void* component, const Json& value)> load;
	std::function<void*(void* component)> address;
};

/// <summary>
/// 登録済みコンポーネント型の情報と、型を知らずに Entity へ付け外し・保存するための関数
/// </summary>
struct ComponentInfo
{
	std::string name;
	std::type_index type = typeid(void);
	std::vector<FieldInfo> fields;
	std::function<bool(ComponentManager&, Entity)> has;
	/// <summary>持っていなければ nullptr</summary>
	std::function<void*(ComponentManager&, Entity)> get;
	/// <summary>既定値に JSON の項目を上書きして付ける。知らない項目名があれば例外</summary>
	std::function<void(ComponentManager&, Entity, const Json&)> addFromJson;
	std::function<Json(ComponentManager&, Entity)> toJson;
	std::function<void(ComponentManager&, Entity)> remove;

	const FieldInfo* findField(std::string_view fieldName) const;
};

class ComponentRegistry
{
private:
	std::vector<ComponentInfo> components;
	ComponentRegistry() = default;
public:
	static ComponentRegistry& getInstance()
	{
		static ComponentRegistry instance;
		return instance;
	}

	template <typename C>
	class Builder
	{
	private:
		ComponentRegistry& registry;
		size_t index;
	public:
		Builder(ComponentRegistry& registry, size_t index) : registry(registry), index(index) {}
		/// <summary>
		/// ゲームが動いている間に System が書き換える値として登録する(保存はするが、エディタでは読むだけ)
		/// </summary>
		template <typename F>
		Builder& runtime(const char* fieldName, F C::* member)
		{
			field(fieldName, member);
			registry.components[index].fields.back().runtime = true;
			return *this;
		}

		template <typename F>
		Builder& field(const char* fieldName, F C::* member)
		{
			FieldInfo f;
			f.name = fieldName;
			f.kind = fieldKindOf<F>();
			if constexpr (NamedEnum<F>)
			{
				for (auto& [e, n] : EnumNames<F>::list()) f.enumNames.push_back(n);
			}
			f.save = [member](const void* c) { return ::toJson(static_cast<const C*>(c)->*member); };
			f.load = [member](void* c, const Json& j) { ::fromJson(j, static_cast<C*>(c)->*member); };
			f.address = [member](void* c) -> void* { return &(static_cast<C*>(c)->*member); };
			registry.components[index].fields.push_back(std::move(f));
			return *this;
		}
	};

	/// <summary>
	/// コンポーネント型を名前付きで登録する。続けて .field("名前", &C::メンバ) で項目を並べる
	/// </summary>
	template <typename C>
	Builder<C> add(const std::string& name)
	{
		ComponentInfo info;
		info.name = name;
		info.type = typeid(C);
		size_t index = components.size();
		info.has = [](ComponentManager& cm, Entity e) { return cm.has<C>(e); };
		info.get = [](ComponentManager& cm, Entity e) -> void* {
			if (!cm.has<C>(e)) return nullptr;
			return &cm.getComponentArray<C>().getComponent(e);
		};
		info.addFromJson = [this, index](ComponentManager& cm, Entity e, const Json& j) {
			const ComponentInfo& self = components[index];
			if (!j.is_object()) throw std::runtime_error(self.name + ": expected an object");
			C value{};
			for (auto& [key, v] : j.items())
			{
				const FieldInfo* f = self.findField(key);
				if (!f) throw std::runtime_error(self.name + ": unknown field \"" + key + "\"");
				try { f->load(&value, v); }
				catch (const std::exception& ex) { throw std::runtime_error(self.name + "." + key + ": " + ex.what()); }
			}
			cm.addComponent<C>(e, value);
		};
		info.toJson = [this, index](ComponentManager& cm, Entity e) {
			const ComponentInfo& self = components[index];
			Json j = Json::object();
			if (!cm.has<C>(e)) return j;
			const C& value = cm.getComponentArray<C>().getComponent(e);
			for (auto& f : self.fields) j[f.name] = f.save(&value);
			return j;
		};
		info.remove = [](ComponentManager& cm, Entity e) { if (cm.has<C>(e)) cm.getComponentArray<C>().removeComponent(e); };
		components.push_back(std::move(info));
		return Builder<C>(*this, index);
	}

	const ComponentInfo* find(std::string_view name) const;
	const ComponentInfo* find(std::type_index type) const;
	inline const std::vector<ComponentInfo>& all() const { return components; }
};

/// <summary>
/// エンジン組み込みのコンポーネントを登録する(何度呼んでも1回だけ)
/// </summary>
void registerBuiltinComponents();
