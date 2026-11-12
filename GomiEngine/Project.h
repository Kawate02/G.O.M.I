#pragma once
#include "JsonConvert.h"

#include <filesystem>
#include <string>

/// <summary>
/// 開いているプロジェクトのフォルダ。形は
///   project.json   名前・使うエンジン(engine。例: "0.1.0")・startScene・pauseUI
///   assets/        シーン・プレハブ・UI・画像・音(アセット ID はここからの相対パス)
///   game/          ゲームのコード(*.cpp / *.h → GomiGame.dll)
///   tests/replays/ 再現テスト(記録の JSON と同名の基準フォルダ)
///   build/         game/ のビルドの出力(コミットしない)
/// </summary>
class Project
{
private:
	std::filesystem::path root;
	Project() = default;
public:
	// DLL をまたいで1つにするため、実体は .cpp に置く
	static Project& getInstance();

	static constexpr const char* FILE_NAME = "project.json";

	/// <summary>
	/// 起動時に開くプロジェクトを決める。指定(--project)> 実行ファイルの隣に project.json があればそこ(書き出したゲーム)
	/// > 開発中のビルドなら見本のプロジェクト > 今のフォルダ
	/// </summary>
	static std::filesystem::path findRoot(const std::string& requested);

	/// <summary>
	/// プロジェクトを開く(assets を AssetDatabase の root にする)
	/// </summary>
	void open(const std::filesystem::path& folder);

	inline const std::filesystem::path& getRoot() const { return root; }
	inline std::filesystem::path file() const { return root / FILE_NAME; }
	inline std::filesystem::path assetsDir() const { return root / "assets"; }
	inline std::filesystem::path gameDir() const { return root / "game"; }
	inline std::filesystem::path replaysDir() const { return root / "tests" / "replays"; }
	inline std::filesystem::path buildDir() const { return root / "build"; }

	/// <summary>
	/// project.json の中身。無ければ空のオブジェクト、壊れていればファイル名入りの例外
	/// </summary>
	Json read() const;

	/// <summary>
	/// project.json の engine(このプロジェクトを開くエンジンのバージョン、例: "0.1.0")。無ければ空
	/// </summary>
	std::string engineVersion() const;
};
