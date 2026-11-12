#pragma once
#include "ISystem.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/// <summary>
/// System を名前で作れるようにする。シーンファイルの "systems" はこの名前を並べる
/// </summary>
class SystemRegistry
{
private:
	std::vector<std::pair<std::string, std::function<std::unique_ptr<ISystem>()>>> factories;
	SystemRegistry() = default;
public:
	// DLL をまたいで1つにするため、実体は .cpp に置く(ヘッダの inline だとモジュールごとに別物になる)
	static SystemRegistry& getInstance();

	template <typename T>
	void add(const std::string& name)
	{
		if (contains(name)) throw std::runtime_error("system \"" + name + "\" is already registered");
		factories.push_back({ name, []() { return std::make_unique<T>(); } });
	}

	bool contains(const std::string& name) const;
	/// <summary>登録した数。ゲームのコード(DLL)を外す時に、読む前の数まで戻すのに使う</summary>
	inline size_t size() const { return factories.size(); }
	/// <summary>count 番目より後の登録を消す(ゲームの DLL を外す前に呼ぶ)</summary>
	void truncate(size_t count);

	/// <summary>
	/// 名前から System を作る。知らない名前なら例外
	/// </summary>
	std::unique_ptr<ISystem> create(const std::string& name) const;
	std::vector<std::string> names() const;
};

/// <summary>
/// エンジン組み込みの System を登録する(何度呼んでも1回だけ)
/// </summary>
void registerBuiltinSystems();
