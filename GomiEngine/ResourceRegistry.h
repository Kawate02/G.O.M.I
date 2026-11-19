#pragma once
#include "ComponentRegistry.h"

#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>

/// <summary>
/// 登録済みの Resource 型(World の外に1つだけある、シーンをまたいで残るデータ)の情報
/// </summary>
struct ResourceInfo
{
	std::string name;
	std::type_index type = typeid(void);
	std::vector<FieldInfo> fields;
	/// <summary>既定値の実体を作る</summary>
	std::function<std::shared_ptr<void>()> create;

	const FieldInfo* findField(std::string_view fieldName) const;
	/// <summary>全項目を { 項目名: 値 } にする</summary>
	Json toJson(const void* value) const;
	/// <summary>書いた項目だけ上書きする。知らない項目名・形の違いは「名前.項目: 理由」の例外</summary>
	void applyJson(void* value, const Json& j) const;
};

/// <summary>
/// Resource 型の登録。ゲームのコードの GOMI_REGISTER { } で resources.add<T>("名前").field(...) と書く
/// </summary>
class ResourceRegistry
{
private:
	std::vector<ResourceInfo> resources;
	ResourceRegistry() = default;
public:
	// DLL をまたいで1つにするため、実体は .cpp に置く
	static ResourceRegistry& getInstance();

	template <typename T>
	FieldBuilder<T> add(const std::string& name)
	{
		if (find(name)) throw std::runtime_error("resource \"" + name + "\" is already registered");
		if (find(std::type_index(typeid(T)))) throw std::runtime_error("resource \"" + name + "\": the type is already registered");
		ResourceInfo info;
		info.name = name;
		info.type = typeid(T);
		info.create = [] { return std::static_pointer_cast<void>(std::make_shared<T>()); };
		size_t index = resources.size();
		resources.push_back(std::move(info));
		return FieldBuilder<T>([this, index]() -> std::vector<FieldInfo>& { return resources[index].fields; });
	}

	const ResourceInfo* find(std::string_view name) const;
	const ResourceInfo* find(std::type_index type) const;
	inline const std::vector<ResourceInfo>& all() const { return resources; }
	inline size_t size() const { return resources.size(); }
	/// <summary>count 番目より後の登録を消す(ゲームの DLL を外す前。先に ResourceStore::clear で値も消しておく)</summary>
	void truncate(size_t count);
};

/// <summary>
/// Resource の今の値の置き場。Game(エディタでは SceneEditor)が1つ持ち、World には参照で渡す。
/// World を作り直しても(シーンを切り替えても)値は残る
/// </summary>
class ResourceStore
{
private:
	std::unordered_map<std::type_index, std::shared_ptr<void>> values;
	void* slot(const ResourceInfo& info);
public:
	/// <summary>
	/// 登録済みの型の値。まだ無ければ既定値で作る。登録されていない型なら例外
	/// </summary>
	template <typename T>
	T& get()
	{
		const ResourceInfo* info = ResourceRegistry::getInstance().find(std::type_index(typeid(T)));
		if (!info) throw std::runtime_error(std::string("resource type ") + typeid(T).name() + " is not registered (add resources.add<T>(\"Name\") in GOMI_REGISTER)");
		return *static_cast<T*>(slot(*info));
	}
	/// <summary>名前で引く(エディタ用)。登録されていなければ nullptr</summary>
	void* find(std::string_view name);
	/// <summary>
	/// 全部を既定値にし、initial({ 名前: { 項目: 値 } })の書いた項目だけ上書きする。
	/// 知らない名前・項目は「source: 名前.項目: 理由」の例外(その時は値を変えない)
	/// </summary>
	void reset(const Json& initial = Json::object(), const std::string& source = "resources");
	/// <summary>
	/// assets/resources.json(無ければ全部既定値)の初期値にする。間違いはファイル名付きの例外
	/// </summary>
	void resetToInitial();
	static constexpr const char* INITIAL_FILE = "resources.json";
	/// <summary>
	/// 全部を既定値にし、state のうち今も登録されている名前・項目だけ戻す(ゲームのコードを差し替えた後。型が変わった項目は既定値のまま)
	/// </summary>
	void restore(const Json& state);
	/// <summary>登録順に { 名前: { 全項目 } }。何も登録されていなければ空のオブジェクト</summary>
	Json save();
	/// <summary>値を全部消す(ゲームの DLL を外す前。値の型の関数が DLL の中にあるため)</summary>
	inline void clear() { values.clear(); }
};
