#pragma once
#include "ISystem.h"

#include <functional>
#include <memory>
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
	static SystemRegistry& getInstance()
	{
		static SystemRegistry instance;
		return instance;
	}

	template <typename T>
	void add(const std::string& name)
	{
		factories.push_back({ name, []() { return std::make_unique<T>(); } });
	}

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
