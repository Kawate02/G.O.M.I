#pragma once
#include <filesystem>
#include <string>

/// <summary>
/// ゲームのコードの DLL(GomiGame.dll)を読んで Component・System を登録し、外す時に登録ごと消す。
/// 外す前に、その DLL の型を持つ World と System を全部消しておくこと(関数の実体が DLL の中にあるため)
/// </summary>
class GameModule
{
private:
	void* handle = nullptr;
	size_t componentBase = 0;
	size_t systemBase = 0;
	std::filesystem::path loadedPath;
public:
	GameModule() = default;
	GameModule(const GameModule&) = delete;
	GameModule& operator=(const GameModule&) = delete;
	~GameModule();

	/// <summary>
	/// DLL を読んで登録する。失敗したら error に理由を入れて false(登録は読む前に戻す)
	/// </summary>
	bool load(const std::filesystem::path& dll, std::string& error);
	void unload();
	inline bool isLoaded() const { return handle != nullptr; }
	inline const std::filesystem::path& path() const { return loadedPath; }

	/// <summary>実行ファイルのあるフォルダ</summary>
	static std::filesystem::path exeDir();
	static constexpr const char* DLL_NAME = "GomiGame.dll";
};
